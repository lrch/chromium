# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Client configuration management.

This module holds the code for detecting and configuring the current client and
it's output directories.
It is responsible for writing out the client specific plugins that tell the
rest of the cr tool what the client is capable of.
"""

import os
import sys

import cr
import cr.auto.build
import cr.auto.client

# The config version currently supported.
VERSION = '0.4'
# The default directory name to store config inside
CLIENT_CONFIG_PATH = '.cr'
# The partial filename to add to a directory to get it's config file.
CLIENT_CONFIG_FILE = os.path.join(CLIENT_CONFIG_PATH, 'config.py')
# The format string for the header of a config file.
CONFIG_FILE_PREFIX = """
# This is an autogenerated file
# it *will* be overwritten, and changes may lost
# The system will autoload any other python file in the same folder.

import cr

OVERRIDES = cr.Config.From("""
# The format string for each value in a config file.
CONFIG_VAR_LINE = '\n  {0} = {1!r},'
# The format string for the tail of a config file.
CONFIG_FILE_SUFFIX = '\n)\n'

# The default config values installed by this module.
DEFAULT = cr.Config.From(
    CR_ROOT_PATH=os.path.join('{GOOGLE_CODE}'),
    CR_CLIENT_PATH=os.path.join('{CR_ROOT_PATH}', '{CR_CLIENT_NAME}'),
    CR_SRC=os.path.join('{CR_CLIENT_PATH}', 'src'),
    CR_BUILD_DIR=os.path.join('{CR_SRC}', '{CR_OUT_FULL}'),
)

# Config values determined at run time by this module.
DETECTED = cr.Config.From(
    CR_CLIENT_PATH=lambda context: _DetectPath(),
    # _DetectName not declared yet so pylint: disable=unnecessary-lambda
    CR_CLIENT_NAME=lambda context: _DetectName(context),
)

_cached_path = None
_cached_name = None


def _DetectPath():
  """A dynamic value function that tries to detect the current client."""
  global _cached_path
  if _cached_path is not None:
    return _cached_path
  # See if we can detect the source tree root
  _cached_path = os.getcwd()
  while (_cached_path and
         not os.path.exists(os.path.join(_cached_path, '.gclient'))):
    old = _cached_path
    _cached_path = os.path.dirname(_cached_path)
    if _cached_path == old:
      _cached_path = None
  if _cached_path is not None:
    dirname, basename = os.path.split(_cached_path)
    if basename == 'src':
      # we have the src path, base is one level up
      _cached_path = dirname
  if _cached_path is None:
    _cached_path = cr.visitor.HIDDEN
  return _cached_path


def _DetectName(context):
  """A dynamic value function that works out the name of the current client."""
  global _cached_name
  if _cached_name is not None:
    return _cached_name
  _cached_name = 'chromium'
  path = context.Get('CR_CLIENT_PATH')
  if path is None:
    return
  _cached_name = os.path.basename(path)
  return _cached_name


def _GetConfigFilename(path):
  return os.path.realpath(os.path.join(path, CLIENT_CONFIG_FILE))


def _IsOutputDir(path):
  return os.path.isfile(_GetConfigFilename(path))


def _WriteConfig(writer, data):
  writer.write(CONFIG_FILE_PREFIX)
  for key, value in data.items():
    writer.write(CONFIG_VAR_LINE.format(key, value))
  writer.write(CONFIG_FILE_SUFFIX)


def AddArguments(parser):
  parser.add_argument(
      '-o', '--out', dest='_out', metavar='name',
      default=None,
      help='The name of the out directory to use. Overrides CR_OUT.'
  )


def GetOutArgument(context):
  return getattr(context.args, '_out', None)


def ApplyOutArgument(context):
  # TODO(iancottrell): be flexible, allow out to do approximate match...
  out = GetOutArgument(context)
  if out:
    context.derived.Set(CR_OUT_FULL=out)


def LoadConfig(context):
  """Loads the client configuration for the given context.

  This will load configuration if present from CR_CLIENT_PATH and then
  CR_BUILD_DIR.

  Args:
    context: The active context to load configuratin for.
  Returns:
    True if configuration was fully loaded.

  """
  # Load the root config, will help set default build dir
  client_path = context.Find('CR_CLIENT_PATH')
  if not client_path:
    return False
  cr.auto.client.__path__.append(os.path.join(client_path, CLIENT_CONFIG_PATH))
  cr.loader.Scan()
  # Now load build dir config
  build_dir = context.Find('CR_BUILD_DIR')
  if not build_dir:
    return False
  cr.auto.build.__path__.append(os.path.join(build_dir, CLIENT_CONFIG_PATH))
  cr.loader.Scan()
  return hasattr(cr.auto.build, 'config')


def WriteConfig(context, path, data):
  """Writes a configuration out to a file.

  This writes all the key value pairs in data out to a config file below path.

  Args:
    context: The context to run under.
    path: The base path to write the config plugin into.
    data: The key value pairs to write.
  """
  filename = _GetConfigFilename(path)
  config_dir = os.path.dirname(filename)
  if context.dry_run:
    print 'makedirs', config_dir
    print 'Write config to', filename
    _WriteConfig(sys.stdout, data)
  else:
    try:
      os.makedirs(config_dir)
    except OSError:
      if not os.path.isdir(config_dir):
        raise
    with open(filename, 'w') as writer:
      _WriteConfig(writer, data)


def PrintInfo(context):
  print 'Selected output directory is', context.Find('CR_BUILD_DIR')
  try:
    for name in cr.auto.build.config.OVERRIDES.exported.keys():
      print ' ', name, '=', context.Get(name)
  except AttributeError:
    pass
