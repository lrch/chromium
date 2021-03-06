// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is auto-generated from
// gpu/command_buffer/build_gles2_cmd_buffer.py
// It's formatted by clang-format using chromium coding style:
//    clang-format -i -style=chromium filename
// DO NOT EDIT!

#ifndef GPU_COMMAND_BUFFER_COMMON_GLES2_CMD_IDS_AUTOGEN_H_
#define GPU_COMMAND_BUFFER_COMMON_GLES2_CMD_IDS_AUTOGEN_H_

#define GLES2_COMMAND_LIST(OP)                           \
  OP(ActiveTexture)                            /* 256 */ \
  OP(AttachShader)                             /* 257 */ \
  OP(BindAttribLocationBucket)                 /* 258 */ \
  OP(BindBuffer)                               /* 259 */ \
  OP(BindBufferBase)                           /* 260 */ \
  OP(BindBufferRange)                          /* 261 */ \
  OP(BindFramebuffer)                          /* 262 */ \
  OP(BindRenderbuffer)                         /* 263 */ \
  OP(BindSampler)                              /* 264 */ \
  OP(BindTexture)                              /* 265 */ \
  OP(BindTransformFeedback)                    /* 266 */ \
  OP(BlendColor)                               /* 267 */ \
  OP(BlendEquation)                            /* 268 */ \
  OP(BlendEquationSeparate)                    /* 269 */ \
  OP(BlendFunc)                                /* 270 */ \
  OP(BlendFuncSeparate)                        /* 271 */ \
  OP(BufferData)                               /* 272 */ \
  OP(BufferSubData)                            /* 273 */ \
  OP(CheckFramebufferStatus)                   /* 274 */ \
  OP(Clear)                                    /* 275 */ \
  OP(ClearBufferfi)                            /* 276 */ \
  OP(ClearBufferfvImmediate)                   /* 277 */ \
  OP(ClearBufferivImmediate)                   /* 278 */ \
  OP(ClearBufferuivImmediate)                  /* 279 */ \
  OP(ClearColor)                               /* 280 */ \
  OP(ClearDepthf)                              /* 281 */ \
  OP(ClearStencil)                             /* 282 */ \
  OP(ColorMask)                                /* 283 */ \
  OP(CompileShader)                            /* 284 */ \
  OP(CompressedTexImage2DBucket)               /* 285 */ \
  OP(CompressedTexImage2D)                     /* 286 */ \
  OP(CompressedTexSubImage2DBucket)            /* 287 */ \
  OP(CompressedTexSubImage2D)                  /* 288 */ \
  OP(CopyBufferSubData)                        /* 289 */ \
  OP(CopyTexImage2D)                           /* 290 */ \
  OP(CopyTexSubImage2D)                        /* 291 */ \
  OP(CreateProgram)                            /* 292 */ \
  OP(CreateShader)                             /* 293 */ \
  OP(CullFace)                                 /* 294 */ \
  OP(DeleteBuffersImmediate)                   /* 295 */ \
  OP(DeleteFramebuffersImmediate)              /* 296 */ \
  OP(DeleteProgram)                            /* 297 */ \
  OP(DeleteRenderbuffersImmediate)             /* 298 */ \
  OP(DeleteSamplersImmediate)                  /* 299 */ \
  OP(DeleteSync)                               /* 300 */ \
  OP(DeleteShader)                             /* 301 */ \
  OP(DeleteTexturesImmediate)                  /* 302 */ \
  OP(DeleteTransformFeedbacksImmediate)        /* 303 */ \
  OP(DepthFunc)                                /* 304 */ \
  OP(DepthMask)                                /* 305 */ \
  OP(DepthRangef)                              /* 306 */ \
  OP(DetachShader)                             /* 307 */ \
  OP(Disable)                                  /* 308 */ \
  OP(DisableVertexAttribArray)                 /* 309 */ \
  OP(DrawArrays)                               /* 310 */ \
  OP(DrawElements)                             /* 311 */ \
  OP(Enable)                                   /* 312 */ \
  OP(EnableVertexAttribArray)                  /* 313 */ \
  OP(FenceSync)                                /* 314 */ \
  OP(Finish)                                   /* 315 */ \
  OP(Flush)                                    /* 316 */ \
  OP(FramebufferRenderbuffer)                  /* 317 */ \
  OP(FramebufferTexture2D)                     /* 318 */ \
  OP(FramebufferTextureLayer)                  /* 319 */ \
  OP(FrontFace)                                /* 320 */ \
  OP(GenBuffersImmediate)                      /* 321 */ \
  OP(GenerateMipmap)                           /* 322 */ \
  OP(GenFramebuffersImmediate)                 /* 323 */ \
  OP(GenRenderbuffersImmediate)                /* 324 */ \
  OP(GenSamplersImmediate)                     /* 325 */ \
  OP(GenTexturesImmediate)                     /* 326 */ \
  OP(GenTransformFeedbacksImmediate)           /* 327 */ \
  OP(GetActiveAttrib)                          /* 328 */ \
  OP(GetActiveUniform)                         /* 329 */ \
  OP(GetAttachedShaders)                       /* 330 */ \
  OP(GetAttribLocation)                        /* 331 */ \
  OP(GetBooleanv)                              /* 332 */ \
  OP(GetBufferParameteriv)                     /* 333 */ \
  OP(GetError)                                 /* 334 */ \
  OP(GetFloatv)                                /* 335 */ \
  OP(GetFragDataLocation)                      /* 336 */ \
  OP(GetFramebufferAttachmentParameteriv)      /* 337 */ \
  OP(GetIntegerv)                              /* 338 */ \
  OP(GetInternalformativ)                      /* 339 */ \
  OP(GetProgramiv)                             /* 340 */ \
  OP(GetProgramInfoLog)                        /* 341 */ \
  OP(GetRenderbufferParameteriv)               /* 342 */ \
  OP(GetSamplerParameterfv)                    /* 343 */ \
  OP(GetSamplerParameteriv)                    /* 344 */ \
  OP(GetShaderiv)                              /* 345 */ \
  OP(GetShaderInfoLog)                         /* 346 */ \
  OP(GetShaderPrecisionFormat)                 /* 347 */ \
  OP(GetShaderSource)                          /* 348 */ \
  OP(GetString)                                /* 349 */ \
  OP(GetTexParameterfv)                        /* 350 */ \
  OP(GetTexParameteriv)                        /* 351 */ \
  OP(GetUniformfv)                             /* 352 */ \
  OP(GetUniformiv)                             /* 353 */ \
  OP(GetUniformLocation)                       /* 354 */ \
  OP(GetVertexAttribfv)                        /* 355 */ \
  OP(GetVertexAttribiv)                        /* 356 */ \
  OP(GetVertexAttribPointerv)                  /* 357 */ \
  OP(Hint)                                     /* 358 */ \
  OP(InvalidateFramebufferImmediate)           /* 359 */ \
  OP(InvalidateSubFramebufferImmediate)        /* 360 */ \
  OP(IsBuffer)                                 /* 361 */ \
  OP(IsEnabled)                                /* 362 */ \
  OP(IsFramebuffer)                            /* 363 */ \
  OP(IsProgram)                                /* 364 */ \
  OP(IsRenderbuffer)                           /* 365 */ \
  OP(IsSampler)                                /* 366 */ \
  OP(IsShader)                                 /* 367 */ \
  OP(IsSync)                                   /* 368 */ \
  OP(IsTexture)                                /* 369 */ \
  OP(IsTransformFeedback)                      /* 370 */ \
  OP(LineWidth)                                /* 371 */ \
  OP(LinkProgram)                              /* 372 */ \
  OP(PauseTransformFeedback)                   /* 373 */ \
  OP(PixelStorei)                              /* 374 */ \
  OP(PolygonOffset)                            /* 375 */ \
  OP(ReadBuffer)                               /* 376 */ \
  OP(ReadPixels)                               /* 377 */ \
  OP(ReleaseShaderCompiler)                    /* 378 */ \
  OP(RenderbufferStorage)                      /* 379 */ \
  OP(ResumeTransformFeedback)                  /* 380 */ \
  OP(SampleCoverage)                           /* 381 */ \
  OP(SamplerParameterf)                        /* 382 */ \
  OP(SamplerParameterfvImmediate)              /* 383 */ \
  OP(SamplerParameteri)                        /* 384 */ \
  OP(SamplerParameterivImmediate)              /* 385 */ \
  OP(Scissor)                                  /* 386 */ \
  OP(ShaderBinary)                             /* 387 */ \
  OP(ShaderSourceBucket)                       /* 388 */ \
  OP(StencilFunc)                              /* 389 */ \
  OP(StencilFuncSeparate)                      /* 390 */ \
  OP(StencilMask)                              /* 391 */ \
  OP(StencilMaskSeparate)                      /* 392 */ \
  OP(StencilOp)                                /* 393 */ \
  OP(StencilOpSeparate)                        /* 394 */ \
  OP(TexImage2D)                               /* 395 */ \
  OP(TexImage3D)                               /* 396 */ \
  OP(TexParameterf)                            /* 397 */ \
  OP(TexParameterfvImmediate)                  /* 398 */ \
  OP(TexParameteri)                            /* 399 */ \
  OP(TexParameterivImmediate)                  /* 400 */ \
  OP(TexStorage3D)                             /* 401 */ \
  OP(TexSubImage2D)                            /* 402 */ \
  OP(TexSubImage3D)                            /* 403 */ \
  OP(Uniform1f)                                /* 404 */ \
  OP(Uniform1fvImmediate)                      /* 405 */ \
  OP(Uniform1i)                                /* 406 */ \
  OP(Uniform1ivImmediate)                      /* 407 */ \
  OP(Uniform1ui)                               /* 408 */ \
  OP(Uniform1uivImmediate)                     /* 409 */ \
  OP(Uniform2f)                                /* 410 */ \
  OP(Uniform2fvImmediate)                      /* 411 */ \
  OP(Uniform2i)                                /* 412 */ \
  OP(Uniform2ivImmediate)                      /* 413 */ \
  OP(Uniform2ui)                               /* 414 */ \
  OP(Uniform2uivImmediate)                     /* 415 */ \
  OP(Uniform3f)                                /* 416 */ \
  OP(Uniform3fvImmediate)                      /* 417 */ \
  OP(Uniform3i)                                /* 418 */ \
  OP(Uniform3ivImmediate)                      /* 419 */ \
  OP(Uniform3ui)                               /* 420 */ \
  OP(Uniform3uivImmediate)                     /* 421 */ \
  OP(Uniform4f)                                /* 422 */ \
  OP(Uniform4fvImmediate)                      /* 423 */ \
  OP(Uniform4i)                                /* 424 */ \
  OP(Uniform4ivImmediate)                      /* 425 */ \
  OP(Uniform4ui)                               /* 426 */ \
  OP(Uniform4uivImmediate)                     /* 427 */ \
  OP(UniformMatrix2fvImmediate)                /* 428 */ \
  OP(UniformMatrix2x3fvImmediate)              /* 429 */ \
  OP(UniformMatrix2x4fvImmediate)              /* 430 */ \
  OP(UniformMatrix3fvImmediate)                /* 431 */ \
  OP(UniformMatrix3x2fvImmediate)              /* 432 */ \
  OP(UniformMatrix3x4fvImmediate)              /* 433 */ \
  OP(UniformMatrix4fvImmediate)                /* 434 */ \
  OP(UniformMatrix4x2fvImmediate)              /* 435 */ \
  OP(UniformMatrix4x3fvImmediate)              /* 436 */ \
  OP(UseProgram)                               /* 437 */ \
  OP(ValidateProgram)                          /* 438 */ \
  OP(VertexAttrib1f)                           /* 439 */ \
  OP(VertexAttrib1fvImmediate)                 /* 440 */ \
  OP(VertexAttrib2f)                           /* 441 */ \
  OP(VertexAttrib2fvImmediate)                 /* 442 */ \
  OP(VertexAttrib3f)                           /* 443 */ \
  OP(VertexAttrib3fvImmediate)                 /* 444 */ \
  OP(VertexAttrib4f)                           /* 445 */ \
  OP(VertexAttrib4fvImmediate)                 /* 446 */ \
  OP(VertexAttribI4i)                          /* 447 */ \
  OP(VertexAttribI4ivImmediate)                /* 448 */ \
  OP(VertexAttribI4ui)                         /* 449 */ \
  OP(VertexAttribI4uivImmediate)               /* 450 */ \
  OP(VertexAttribIPointer)                     /* 451 */ \
  OP(VertexAttribPointer)                      /* 452 */ \
  OP(Viewport)                                 /* 453 */ \
  OP(BlitFramebufferCHROMIUM)                  /* 454 */ \
  OP(RenderbufferStorageMultisampleCHROMIUM)   /* 455 */ \
  OP(RenderbufferStorageMultisampleEXT)        /* 456 */ \
  OP(FramebufferTexture2DMultisampleEXT)       /* 457 */ \
  OP(TexStorage2DEXT)                          /* 458 */ \
  OP(GenQueriesEXTImmediate)                   /* 459 */ \
  OP(DeleteQueriesEXTImmediate)                /* 460 */ \
  OP(BeginQueryEXT)                            /* 461 */ \
  OP(BeginTransformFeedback)                   /* 462 */ \
  OP(EndQueryEXT)                              /* 463 */ \
  OP(EndTransformFeedback)                     /* 464 */ \
  OP(InsertEventMarkerEXT)                     /* 465 */ \
  OP(PushGroupMarkerEXT)                       /* 466 */ \
  OP(PopGroupMarkerEXT)                        /* 467 */ \
  OP(GenVertexArraysOESImmediate)              /* 468 */ \
  OP(DeleteVertexArraysOESImmediate)           /* 469 */ \
  OP(IsVertexArrayOES)                         /* 470 */ \
  OP(BindVertexArrayOES)                       /* 471 */ \
  OP(SwapBuffers)                              /* 472 */ \
  OP(GetMaxValueInBufferCHROMIUM)              /* 473 */ \
  OP(EnableFeatureCHROMIUM)                    /* 474 */ \
  OP(ResizeCHROMIUM)                           /* 475 */ \
  OP(GetRequestableExtensionsCHROMIUM)         /* 476 */ \
  OP(RequestExtensionCHROMIUM)                 /* 477 */ \
  OP(GetProgramInfoCHROMIUM)                   /* 478 */ \
  OP(GetTranslatedShaderSourceANGLE)           /* 479 */ \
  OP(PostSubBufferCHROMIUM)                    /* 480 */ \
  OP(TexImageIOSurface2DCHROMIUM)              /* 481 */ \
  OP(CopyTextureCHROMIUM)                      /* 482 */ \
  OP(DrawArraysInstancedANGLE)                 /* 483 */ \
  OP(DrawElementsInstancedANGLE)               /* 484 */ \
  OP(VertexAttribDivisorANGLE)                 /* 485 */ \
  OP(GenMailboxCHROMIUM)                       /* 486 */ \
  OP(ProduceTextureCHROMIUMImmediate)          /* 487 */ \
  OP(ProduceTextureDirectCHROMIUMImmediate)    /* 488 */ \
  OP(ConsumeTextureCHROMIUMImmediate)          /* 489 */ \
  OP(CreateAndConsumeTextureCHROMIUMImmediate) /* 490 */ \
  OP(BindUniformLocationCHROMIUMBucket)        /* 491 */ \
  OP(GenValuebuffersCHROMIUMImmediate)         /* 492 */ \
  OP(DeleteValuebuffersCHROMIUMImmediate)      /* 493 */ \
  OP(IsValuebufferCHROMIUM)                    /* 494 */ \
  OP(BindValuebufferCHROMIUM)                  /* 495 */ \
  OP(SubscribeValueCHROMIUM)                   /* 496 */ \
  OP(PopulateSubscribedValuesCHROMIUM)         /* 497 */ \
  OP(UniformValuebufferCHROMIUM)               /* 498 */ \
  OP(BindTexImage2DCHROMIUM)                   /* 499 */ \
  OP(ReleaseTexImage2DCHROMIUM)                /* 500 */ \
  OP(TraceBeginCHROMIUM)                       /* 501 */ \
  OP(TraceEndCHROMIUM)                         /* 502 */ \
  OP(AsyncTexSubImage2DCHROMIUM)               /* 503 */ \
  OP(AsyncTexImage2DCHROMIUM)                  /* 504 */ \
  OP(WaitAsyncTexImage2DCHROMIUM)              /* 505 */ \
  OP(WaitAllAsyncTexImage2DCHROMIUM)           /* 506 */ \
  OP(DiscardFramebufferEXTImmediate)           /* 507 */ \
  OP(LoseContextCHROMIUM)                      /* 508 */ \
  OP(InsertSyncPointCHROMIUM)                  /* 509 */ \
  OP(WaitSyncPointCHROMIUM)                    /* 510 */ \
  OP(DrawBuffersEXTImmediate)                  /* 511 */ \
  OP(DiscardBackbufferCHROMIUM)                /* 512 */ \
  OP(ScheduleOverlayPlaneCHROMIUM)             /* 513 */ \
  OP(SwapInterval)                             /* 514 */ \
  OP(MatrixLoadfCHROMIUMImmediate)             /* 515 */ \
  OP(MatrixLoadIdentityCHROMIUM)               /* 516 */ \
  OP(BlendBarrierKHR)                          /* 517 */

enum CommandId {
  kStartPoint = cmd::kLastCommonId,  // All GLES2 commands start after this.
#define GLES2_CMD_OP(name) k##name,
  GLES2_COMMAND_LIST(GLES2_CMD_OP)
#undef GLES2_CMD_OP
      kNumCommands
};

#endif  // GPU_COMMAND_BUFFER_COMMON_GLES2_CMD_IDS_AUTOGEN_H_
