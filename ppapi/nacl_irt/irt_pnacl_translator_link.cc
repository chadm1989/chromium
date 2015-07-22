// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/srpc/nacl_srpc.h"
#include "native_client/src/untrusted/irt/irt_dev.h"
#include "ppapi/nacl_irt/irt_interfaces.h"

namespace {

const int kMaxObjectFiles = 16;

int (*g_func)(int nexe_fd,
              const int* obj_file_fds,
              int obj_file_fd_count);

void HandleLinkRequest(NaClSrpcRpc* rpc,
                       NaClSrpcArg** in_args,
                       NaClSrpcArg** out_args,
                       NaClSrpcClosure* done) {
  int obj_file_count = in_args[0]->u.ival;
  int nexe_fd = in_args[kMaxObjectFiles + 1]->u.hval;

  if (obj_file_count < 1 || obj_file_count > kMaxObjectFiles) {
    NaClLog(LOG_FATAL, "Bad object file count (%i)\n", obj_file_count);
  }
  int obj_file_fds[obj_file_count];
  for (int i = 0; i < obj_file_count; i++) {
    obj_file_fds[i] = in_args[i + 1]->u.hval;
  }

  int result = g_func(nexe_fd, obj_file_fds, obj_file_count);

  rpc->result = result == 0 ? NACL_SRPC_RESULT_OK : NACL_SRPC_RESULT_APP_ERROR;
  done->Run(done);
}

const struct NaClSrpcHandlerDesc kSrpcMethods[] = {
  { "RunWithSplit:ihhhhhhhhhhhhhhhhh:", HandleLinkRequest },
  { NULL, NULL },
};

void ServeLinkRequest(int (*func)(int nexe_fd,
                                  const int* obj_file_fds,
                                  int obj_file_fd_count)) {
  g_func = func;
  if (!NaClSrpcModuleInit()) {
    NaClLog(LOG_FATAL, "NaClSrpcModuleInit() failed\n");
  }
  if (!NaClSrpcAcceptClientConnection(kSrpcMethods)) {
    NaClLog(LOG_FATAL, "NaClSrpcAcceptClientConnection() failed\n");
  }
}

}

const struct nacl_irt_private_pnacl_translator_link
    nacl_irt_private_pnacl_translator_link = {
  ServeLinkRequest
};
