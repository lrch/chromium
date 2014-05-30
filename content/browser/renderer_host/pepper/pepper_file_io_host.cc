// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/pepper_file_io_host.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/files/file_util_proxy.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/renderer_host/pepper/pepper_file_ref_host.h"
#include "content/browser/renderer_host/pepper/pepper_file_system_browser_host.h"
#include "content/browser/renderer_host/pepper/pepper_security_helper.h"
#include "content/common/fileapi/file_system_messages.h"
#include "content/common/sandbox_util.h"
#include "content/common/view_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_client.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/ppb_file_io.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/shared_impl/file_system_util.h"
#include "ppapi/shared_impl/file_type_conversion.h"
#include "ppapi/shared_impl/time_conversion.h"
#include "webkit/browser/fileapi/file_observers.h"
#include "webkit/browser/fileapi/file_system_context.h"
#include "webkit/browser/fileapi/file_system_operation_runner.h"
#include "webkit/browser/fileapi/task_runner_bound_observer_list.h"
#include "webkit/common/fileapi/file_system_util.h"

namespace content {

using ppapi::FileIOStateManager;
using ppapi::PPTimeToTime;

namespace {

PepperFileIOHost::UIThreadStuff GetUIThreadStuffForInternalFileSystems(
    int render_process_id) {
  PepperFileIOHost::UIThreadStuff stuff;
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  RenderProcessHost* host = RenderProcessHost::FromID(render_process_id);
  if (host) {
    stuff.resolved_render_process_id = base::GetProcId(host->GetHandle());
    StoragePartition* storage_partition = host->GetStoragePartition();
    if (storage_partition)
      stuff.file_system_context = storage_partition->GetFileSystemContext();
  }
  return stuff;
}

base::ProcessId GetResolvedRenderProcessId(int render_process_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  RenderProcessHost* host = RenderProcessHost::FromID(render_process_id);
  if (!host)
    return base::kNullProcessId;
  return base::GetProcId(host->GetHandle());
}

bool GetPluginAllowedToCallRequestOSFileHandle(int render_process_id,
                                               const GURL& document_url) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ContentBrowserClient* client = GetContentClient()->browser();
  RenderProcessHost* host = RenderProcessHost::FromID(render_process_id);
  if (!host)
    return false;
  return client->IsPluginAllowedToCallRequestOSFileHandle(
      host->GetBrowserContext(), document_url);
}

bool FileOpenForWrite(int32_t open_flags) {
  return (open_flags & (PP_FILEOPENFLAG_WRITE | PP_FILEOPENFLAG_APPEND)) != 0;
}

}  // namespace

PepperFileIOHost::PepperFileIOHost(BrowserPpapiHostImpl* host,
                                   PP_Instance instance,
                                   PP_Resource resource)
    : ResourceHost(host->GetPpapiHost(), instance, resource),
      browser_ppapi_host_(host),
      render_process_host_(NULL),
      file_(BrowserThread::GetMessageLoopProxyForThread(BrowserThread::FILE)),
      open_flags_(0),
      file_system_type_(PP_FILESYSTEMTYPE_INVALID),
      max_written_offset_(0),
      check_quota_(false),
      weak_factory_(this) {
  int unused;
  if (!host->GetRenderFrameIDsForInstance(
          instance, &render_process_id_, &unused)) {
    render_process_id_ = -1;
  }
}

PepperFileIOHost::~PepperFileIOHost() {}

int32_t PepperFileIOHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperFileIOHost, msg)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_FileIO_Open, OnHostMsgOpen)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_FileIO_Touch, OnHostMsgTouch)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_FileIO_SetLength,
                                      OnHostMsgSetLength)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_FileIO_Flush,
                                        OnHostMsgFlush)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_FileIO_Close, OnHostMsgClose)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_FileIO_RequestOSFileHandle,
                                        OnHostMsgRequestOSFileHandle)
  PPAPI_END_MESSAGE_MAP()
  return PP_ERROR_FAILED;
}

PepperFileIOHost::UIThreadStuff::UIThreadStuff() {
  resolved_render_process_id = base::kNullProcessId;
}

PepperFileIOHost::UIThreadStuff::~UIThreadStuff() {}

int32_t PepperFileIOHost::OnHostMsgOpen(
    ppapi::host::HostMessageContext* context,
    PP_Resource file_ref_resource,
    int32_t open_flags) {
  int32_t rv = state_manager_.CheckOperationState(
      FileIOStateManager::OPERATION_EXCLUSIVE, false);
  if (rv != PP_OK)
    return rv;

  int platform_file_flags = 0;
  if (!ppapi::PepperFileOpenFlagsToPlatformFileFlags(open_flags,
                                                     &platform_file_flags))
    return PP_ERROR_BADARGUMENT;

  ppapi::host::ResourceHost* resource_host =
      host()->GetResourceHost(file_ref_resource);
  if (!resource_host || !resource_host->IsFileRefHost())
    return PP_ERROR_BADRESOURCE;
  PepperFileRefHost* file_ref_host =
      static_cast<PepperFileRefHost*>(resource_host);
  if (file_ref_host->GetFileSystemType() == PP_FILESYSTEMTYPE_INVALID)
    return PP_ERROR_FAILED;

  file_system_host_ = file_ref_host->GetFileSystemHost();

  open_flags_ = open_flags;
  file_system_type_ = file_ref_host->GetFileSystemType();
  file_system_url_ = file_ref_host->GetFileSystemURL();

  // For external file systems, if there is a valid FileSystemURL, then treat
  // it like internal file systems and access it via the FileSystemURL.
  bool is_internal_type = (file_system_type_ != PP_FILESYSTEMTYPE_EXTERNAL) ||
                          file_system_url_.is_valid();

  if (is_internal_type) {
    if (!file_system_url_.is_valid())
      return PP_ERROR_BADARGUMENT;

    // Not all external file systems are fully supported yet.
    // Whitelist the supported ones.
    if (file_system_url_.mount_type() == fileapi::kFileSystemTypeExternal) {
      switch (file_system_url_.type()) {
        case fileapi::kFileSystemTypeNativeMedia:
        case fileapi::kFileSystemTypeDeviceMedia:
        case fileapi::kFileSystemTypePicasa:
        case fileapi::kFileSystemTypeItunes:
        case fileapi::kFileSystemTypeIphoto:
          break;
        default:
          return PP_ERROR_NOACCESS;
      }
    }
    if (!CanOpenFileSystemURLWithPepperFlags(
            open_flags, render_process_id_, file_system_url_))
      return PP_ERROR_NOACCESS;
    BrowserThread::PostTaskAndReplyWithResult(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&GetUIThreadStuffForInternalFileSystems, render_process_id_),
        base::Bind(&PepperFileIOHost::GotUIThreadStuffForInternalFileSystems,
                   weak_factory_.GetWeakPtr(),
                   context->MakeReplyMessageContext(),
                   platform_file_flags));
  } else {
    base::FilePath path = file_ref_host->GetExternalFilePath();
    if (!CanOpenWithPepperFlags(open_flags, render_process_id_, path))
      return PP_ERROR_NOACCESS;
    BrowserThread::PostTaskAndReplyWithResult(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&GetResolvedRenderProcessId, render_process_id_),
        base::Bind(&PepperFileIOHost::GotResolvedRenderProcessId,
                   weak_factory_.GetWeakPtr(),
                   context->MakeReplyMessageContext(),
                   path,
                   platform_file_flags));
  }
  state_manager_.SetPendingOperation(FileIOStateManager::OPERATION_EXCLUSIVE);
  return PP_OK_COMPLETIONPENDING;
}

void PepperFileIOHost::GotUIThreadStuffForInternalFileSystems(
    ppapi::host::ReplyMessageContext reply_context,
    int platform_file_flags,
    UIThreadStuff ui_thread_stuff) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  file_system_context_ = ui_thread_stuff.file_system_context;
  resolved_render_process_id_ = ui_thread_stuff.resolved_render_process_id;
  if (resolved_render_process_id_ == base::kNullProcessId ||
      !file_system_context_.get()) {
    reply_context.params.set_result(PP_ERROR_FAILED);
    SendOpenErrorReply(reply_context);
    return;
  }

  if (!file_system_context_->GetFileSystemBackend(file_system_url_.type())) {
    reply_context.params.set_result(PP_ERROR_FAILED);
    SendOpenErrorReply(reply_context);
    return;
  }

  DCHECK(file_system_host_.get());
  DCHECK(file_system_host_->GetFileSystemOperationRunner());
  file_system_host_->GetFileSystemOperationRunner()->OpenFile(
      file_system_url_,
      platform_file_flags,
      base::Bind(&PepperFileIOHost::DidOpenInternalFile,
                 weak_factory_.GetWeakPtr(),
                 reply_context));
}

void PepperFileIOHost::DidOpenInternalFile(
    ppapi::host::ReplyMessageContext reply_context,
    base::File file,
    const base::Closure& on_close_callback) {
  if (file.IsValid()) {
    on_close_callback_ = on_close_callback;

    if (FileOpenForWrite(open_flags_) && file_system_host_->ChecksQuota()) {
      check_quota_ = true;
      file_system_host_->OpenQuotaFile(
          this,
          file_system_url_,
          base::Bind(&PepperFileIOHost::DidOpenQuotaFile,
                     weak_factory_.GetWeakPtr(),
                     reply_context,
                     base::Passed(&file)));
      return;
    }
  }

  DCHECK(!file_.IsValid());
  base::File::Error error =
      file.IsValid() ? base::File::FILE_OK : file.error_details();
  file_.SetFile(file.Pass());
  OnOpenProxyCallback(reply_context, error);
}

void PepperFileIOHost::GotResolvedRenderProcessId(
    ppapi::host::ReplyMessageContext reply_context,
    base::FilePath path,
    int file_flags,
    base::ProcessId resolved_render_process_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  resolved_render_process_id_ = resolved_render_process_id;
  file_.CreateOrOpen(
      path,
      file_flags,
      base::Bind(&PepperFileIOHost::OnOpenProxyCallback,
                 weak_factory_.GetWeakPtr(),
                 reply_context));
}

int32_t PepperFileIOHost::OnHostMsgTouch(
    ppapi::host::HostMessageContext* context,
    PP_Time last_access_time,
    PP_Time last_modified_time) {
  int32_t rv = state_manager_.CheckOperationState(
      FileIOStateManager::OPERATION_EXCLUSIVE, true);
  if (rv != PP_OK)
    return rv;

  if (!file_.SetTimes(
          PPTimeToTime(last_access_time),
          PPTimeToTime(last_modified_time),
          base::Bind(&PepperFileIOHost::ExecutePlatformGeneralCallback,
                     weak_factory_.GetWeakPtr(),
                     context->MakeReplyMessageContext()))) {
    return PP_ERROR_FAILED;
  }

  state_manager_.SetPendingOperation(FileIOStateManager::OPERATION_EXCLUSIVE);
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperFileIOHost::OnHostMsgSetLength(
    ppapi::host::HostMessageContext* context,
    int64_t length) {
  int32_t rv = state_manager_.CheckOperationState(
      FileIOStateManager::OPERATION_EXCLUSIVE, true);
  if (rv != PP_OK)
    return rv;
  if (length < 0)
    return PP_ERROR_BADARGUMENT;

  // Quota checks are performed on the plugin side, in order to use the same
  // quota reservation and request system as Write.

  if (!file_.SetLength(
          length,
          base::Bind(&PepperFileIOHost::ExecutePlatformGeneralCallback,
                     weak_factory_.GetWeakPtr(),
                     context->MakeReplyMessageContext()))) {
    return PP_ERROR_FAILED;
  }

  state_manager_.SetPendingOperation(FileIOStateManager::OPERATION_EXCLUSIVE);
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperFileIOHost::OnHostMsgFlush(
    ppapi::host::HostMessageContext* context) {
  int32_t rv = state_manager_.CheckOperationState(
      FileIOStateManager::OPERATION_EXCLUSIVE, true);
  if (rv != PP_OK)
    return rv;

  if (!file_.Flush(
          base::Bind(&PepperFileIOHost::ExecutePlatformGeneralCallback,
                     weak_factory_.GetWeakPtr(),
                     context->MakeReplyMessageContext()))) {
    return PP_ERROR_FAILED;
  }

  state_manager_.SetPendingOperation(FileIOStateManager::OPERATION_EXCLUSIVE);
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperFileIOHost::OnHostMsgClose(
    ppapi::host::HostMessageContext* context,
    const ppapi::FileGrowth& file_growth) {
  if (check_quota_) {
    file_system_host_->CloseQuotaFile(this, file_growth);
    check_quota_ = false;
  }

  if (file_.IsValid()) {
    file_.Close(base::Bind(&PepperFileIOHost::DidCloseFile,
                           weak_factory_.GetWeakPtr()));
  }
  return PP_OK;
}

void PepperFileIOHost::DidOpenQuotaFile(
    ppapi::host::ReplyMessageContext reply_context,
    base::File file,
    int64_t max_written_offset) {
  DCHECK(!file_.IsValid());
  DCHECK(file.IsValid());
  max_written_offset_ = max_written_offset;
  file_.SetFile(file.Pass());

  OnOpenProxyCallback(reply_context, base::File::FILE_OK);
}

void PepperFileIOHost::DidCloseFile(base::File::Error /*error*/) {
  // Silently ignore if we fail to close the file.
  if (!on_close_callback_.is_null()) {
    on_close_callback_.Run();
    on_close_callback_.Reset();
  }
}

int32_t PepperFileIOHost::OnHostMsgRequestOSFileHandle(
    ppapi::host::HostMessageContext* context) {
  if (open_flags_ != PP_FILEOPENFLAG_READ && file_system_host_->ChecksQuota())
    return PP_ERROR_FAILED;

  GURL document_url =
      browser_ppapi_host_->GetDocumentURLForInstance(pp_instance());
  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::UI,
      FROM_HERE,
      base::Bind(&GetPluginAllowedToCallRequestOSFileHandle,
                 render_process_id_,
                 document_url),
      base::Bind(&PepperFileIOHost::GotPluginAllowedToCallRequestOSFileHandle,
                 weak_factory_.GetWeakPtr(),
                 context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

void PepperFileIOHost::GotPluginAllowedToCallRequestOSFileHandle(
    ppapi::host::ReplyMessageContext reply_context,
    bool plugin_allowed) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!browser_ppapi_host_->external_plugin() ||
      host()->permissions().HasPermission(ppapi::PERMISSION_PRIVATE) ||
      plugin_allowed) {
    if (!AddFileToReplyContext(open_flags_, &reply_context))
      reply_context.params.set_result(PP_ERROR_FAILED);
  } else {
    reply_context.params.set_result(PP_ERROR_NOACCESS);
  }
  host()->SendReply(reply_context,
                    PpapiPluginMsg_FileIO_RequestOSFileHandleReply());
}

void PepperFileIOHost::ExecutePlatformGeneralCallback(
    ppapi::host::ReplyMessageContext reply_context,
    base::File::Error error_code) {
  reply_context.params.set_result(ppapi::FileErrorToPepperError(error_code));
  host()->SendReply(reply_context, PpapiPluginMsg_FileIO_GeneralReply());
  state_manager_.SetOperationFinished();
}

void PepperFileIOHost::OnOpenProxyCallback(
    ppapi::host::ReplyMessageContext reply_context,
    base::File::Error error_code) {
  int32_t pp_error = ppapi::FileErrorToPepperError(error_code);
  if (file_.IsValid() && !AddFileToReplyContext(open_flags_, &reply_context))
    pp_error = PP_ERROR_FAILED;

  PP_Resource quota_file_system = 0;
  if (pp_error == PP_OK) {
    state_manager_.SetOpenSucceed();
    // A non-zero resource id signals the plugin side to check quota.
    if (check_quota_)
      quota_file_system = file_system_host_->pp_resource();
  }

  reply_context.params.set_result(pp_error);
  host()->SendReply(
      reply_context,
      PpapiPluginMsg_FileIO_OpenReply(quota_file_system, max_written_offset_));
  state_manager_.SetOperationFinished();
}

void PepperFileIOHost::SendOpenErrorReply(
    ppapi::host::ReplyMessageContext reply_context) {
  host()->SendReply(reply_context, PpapiPluginMsg_FileIO_OpenReply(0, 0));
}

bool PepperFileIOHost::AddFileToReplyContext(
    int32_t open_flags,
    ppapi::host::ReplyMessageContext* reply_context) const {
  base::ProcessId plugin_process_id =
      base::GetProcId(browser_ppapi_host_->GetPluginProcessHandle());
  if (plugin_process_id == base::kNullProcessId)
    plugin_process_id = resolved_render_process_id_;

  IPC::PlatformFileForTransit transit_file =
      BrokerGetFileHandleForProcess(file_.GetPlatformFile(), plugin_process_id,
                                    false);
  if (transit_file == IPC::InvalidPlatformFileForTransit())
    return false;

  ppapi::proxy::SerializedHandle file_handle;
  // A non-zero resource id signals NaClIPCAdapter to create a NaClQuotaDesc.
  PP_Resource quota_file_io = check_quota_ ? pp_resource() : 0;
  file_handle.set_file_handle(transit_file, open_flags, quota_file_io);
  reply_context->params.AppendHandle(file_handle);
  return true;
}

}  // namespace content
