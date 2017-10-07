/*
 *  Copyright (c) 2016, https://github.com/zhatalk
 *  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NUBULA_NET_HANDLER_MTPROTO_OBJECT_HANDLER_H_
#define NUBULA_NET_HANDLER_MTPROTO_OBJECT_HANDLER_H_

#include "nebula/net/telegram/core/mtproto_message.h"

#include "nebula/net/handler/nebula_base_handler.h"
#include "nebula/net/handler/nebula_event_callback.h"

using TelegramEventCallback = NebulaEventCallback<nebula::ZProtoPipeline, std::shared_ptr<MTProtoBase>>;

class MTProtoDecoder;
class TelegramHandler : public wangle::HandlerAdapter<
    std::unique_ptr<folly::IOBuf>, std::unique_ptr<folly::IOBuf>
  >, public nebula::NebulaBaseHandler
{
public:
  enum ConnState {
    NONE = 0,
    CONNECTED,                     // 连接建立
    WAIT_TL_req_pq,                // 等TL_req_pq
    WAIT_TL_req_DH_params,         // 等req_DH_params
    WAIT_TL_set_client_DH_params,  // 等TL_set_client_DH_params
    RPC_RUNNING,                   // running
    CLOSED,
    ERROR
  };

  explicit TelegramHandler(nebula::ServiceBase* service)
    : NebulaBaseHandler(service) {
    // a_ = BN_new();
    // DCHECK(a_);
  }

  virtual ~TelegramHandler() {
    if (a_) BN_free(a_);
    if (p_) BN_free(p_);
  }
  
  //////////////////////////////////////////////////////////////////////////
  // 重载 HandlerAdapter<std::unique_ptr<IOBuf>>
  void read(Context* ctx, std::unique_ptr<folly::IOBuf> msg) override;
  folly::Future<folly::Unit> write(Context* ctx, std::unique_ptr<folly::IOBuf> out) override;
  
  void readEOF(Context* ctx) override;
  void readException(Context* ctx, folly::exception_wrapper e) override;
  
  void transportActive(Context* ctx) override;
  void transportInactive(Context* ctx) override;
  
  folly::Future<folly::Unit> close(Context* ctx) override;
  
//  const TLInt2048& GetAuthKey() const {
//    return auth_key_;
//  }
//
//  int64_t GetAuthKeyID() const {
//    return auth_key_id_;
//  }

private:
  // OnMTProto
  void OnMTProtoQuickAck(Context* ctx, std::shared_ptr<MTProtoQuickAck> mproto);
  void OnMTProtoUnencryptedMessage(Context* ctx, std::shared_ptr<MTProtoUnencryptedMessage> mproto);
  void OnMTProtoEncryptedMessage(Context* ctx, std::shared_ptr<MTProtoEncryptedMessage> mproto);

  // handshake
  TLObjectPtr OnHandshake(Context* ctx, TLObjectPtr msg);
  
  TLObjectPtr On_TL_req_pq(Context* ctx, TLObjectPtr msg);
  TLObjectPtr On_TL_req_DH_params(Context* ctx, TLObjectPtr msg);
  TLObjectPtr On_TL_set_client_DH_params(Context* ctx, TLObjectPtr msg);

  // rpc
  TLObjectPtr OnRpcData(Context* ctx, const TLObject* msg);

  TLObjectPtr On_TL_invokeWithLayer(Context* ctx, const TLObject* msg);
  
  TLObjectPtr On_TL_ping_delay_disconnect(Context* ctx, const TLObject* msg);
  TLObjectPtr On_TL_ping(Context* ctx, const TLObject* msg);
  
  TLObjectPtr On_TL_msg_container(Context* ctx, const TLObject* msg);
  
  TLObjectPtr On_TL_destroy_session(Context* ctx, const TLObject* msg);
  TLObjectPtr On_TL_initConnection(Context* ctx, const TLObject* msg);

  TLObjectPtr On_TL_help_getConfig(Context* ctx, const TLObject* msg);
  TLObjectPtr On_TL_auth_sendCode(Context* ctx, const TLObject* msg);
  TLObjectPtr On_TL_auth_signIn(Context* ctx, const TLObject* msg);

  // TODO(@benqi): s2s应用场景里，连接发起方需要保活，逻辑基本一样，
  //  后续zrpc_client_handler/zproto_handler等统一处理
  static void DoHeartBeat(uint64_t conn_id, uint32_t timeout);
  
  uint64_t conn_id_ {0};
  int conn_state_ {NONE};
  
  std::string remote_address_;
  
  // 客户端发上来的
  TLInt128 nonce_;
  // 服务端生成的
  TLInt128 server_nonce_;
  TLInt256 new_nonce_;

  // TLString g_a_;
  BIGNUM *a_ {nullptr}; //  = BN_new();
  BIGNUM *p_ {nullptr}; //  = BN_new();
  // DCHECK(g_a);

  std::shared_ptr<TLInt2048> auth_key_;
  int64_t auth_key_id_ {0};

  // MTProtoDecoder* mtproto_decoder_ {nullptr};
  
  bool first_recved_ {true};
  int64_t session_id_ {0};
};

void ModuleTelegramInitialize();

#endif
