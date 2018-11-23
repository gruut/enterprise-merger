#include "grpc_merger.hpp"
#include "../../utils/compressor.hpp"
#include "grpc_util.hpp"
#include <chrono>
#include <thread>

namespace gruut {

void MergerRpcServer::runMergerServ(char *port_for_merger) {
  std::string port_num(port_for_merger);
  std::string server_address("0.0.0.0:");
  server_address += port_num;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

  builder.RegisterService(&m_service_merger);
  m_cq_merger = builder.AddCompletionQueue();
  m_server_merger = builder.BuildAndStart();
  std::cout << "Server listening on " << server_address << " for Merger"
			<< std::endl;
  handleMergerRpcs();
}
void MergerRpcServer::runSignerServ(char *port_for_signer) {
  std::string port_num(port_for_signer);
  std::string server_address("0.0.0.0:");
  server_address += port_num;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService (&m_service_signer);
  m_server_signer = builder.BuildAndStart();
  std::cout << "Server listening on " << server_address << " for Signer" << std::endl;
  m_server_signer->Wait();
}

void MergerRpcServer::ReceiveData::proceed() {
  switch (m_receive_status) {
  case CallStatus::CREATE: {
	m_receive_status = CallStatus::PROCESS;
	m_service->RequestpushData(&m_ctx, &m_request, &m_responder, m_cq, m_cq,
							   this);
  } break;

  case CallStatus::PROCESS: {
	new ReceiveData(m_service, m_cq);

	Status st;
	std::string raw_data = m_request.data();
	MessageHeader msg_header = HeaderController::parseHeader(raw_data);
	if (!HeaderController::validateMessage(msg_header)) {
	  m_reply.set_checker(false);
	} else {
	  int json_size = HeaderController::getJsonSize(msg_header);
	  std::string compressed_data = HeaderController::detachHeader(raw_data);
	  std::string decompressed_data;
	  // TODO: MAC verification 필요

	  Compressor::decompressData(compressed_data, decompressed_data, json_size);

	  Message msg(msg_header);
	  msg.data = nlohmann::json::parse(decompressed_data);

	  if(JsonValidator::validateSchema(msg.data, msg.message_type)) {
		auto input_queue  = Application::app().getInputQueue();
		input_queue->push(msg);
		m_reply.set_checker(true);
		st = Status::OK;
	  }
	  else {
		m_reply.set_checker(false);
		st = Status(StatusCode::CANCELLED , "Json schema check fail");
	  }
	}
	m_receive_status = CallStatus::FINISH;
	m_responder.Finish(m_reply, st, this);
  }
	break;

  default: {
	GPR_ASSERT(m_receive_status == CallStatus::FINISH);
	delete this;
  }
	break;
  }
}

void MergerRpcServer::handleMergerRpcs(){
  new ReceiveData(&m_service_merger, m_cq_merger.get());
  void *tag;
  bool ok;
  while (true) {
	GPR_ASSERT(m_cq_merger->Next(&tag, &ok));
	GPR_ASSERT(ok);
	static_cast<CallData *>(tag)->proceed();
  }
}
bool MergerRpcServer::checkSignerMsgType(MessageType msg_type){
  return (msg_type == MessageType::MSG_CHALLENGE ||
	  msg_type == MessageType::MSG_RESPONSE_SECOND ||
	  msg_type == MessageType::MSG_ACCEPT ||
	  msg_type == MessageType::MSG_ECHO ||
	  msg_type == MessageType::MSG_ERROR);
}
void MergerRpcServer::sendDataToSigner(std::string &header_added_data, uint64_t receiver_id, MessageType msg_type){
  switch(msg_type){
  case MessageType::MSG_CHALLENGE:{
	m_receiver_list[receiver_id].msg_challenge->set_message(header_added_data);
	m_receiver_list[receiver_id].msg_challenge = nullptr;
  }
	break;
  case MessageType::MSG_ACCEPT:{
	m_receiver_list[receiver_id].msg_accept->set_message(header_added_data);
	m_receiver_list[receiver_id].msg_accept = nullptr;
  }
	break;
  case MessageType::MSG_RESPONSE_SECOND:{
	m_receiver_list[receiver_id].msg_response2->set_message(header_added_data);
	m_receiver_list[receiver_id].msg_response2 = nullptr;
  }
	break;
  case MessageType::MSG_REQ_SSIG:{
	GrpcMsgReqSsig msg;
	msg.set_message(header_added_data);
	m_receiver_list[receiver_id].stream->Write(msg);
  }
	break;
  default:
	break;
  }
}

Status MergerRpcServer::SignerService::openChannel (ServerContext *context,
													ServerReaderWriter<GrpcMsgReqSsig, Identity> *stream) {
  ReceiverRpcData receiver_rpc = ReceiverRpcData();
  receiver_rpc.stream = stream;
  uint64_t receiver_id;
  Identity receiver_data;
  stream->Read(&receiver_data);
  memcpy(&receiver_id, &receiver_data.sender()[0], sizeof(uint64_t));
  m_server.m_receiver_list[receiver_id] = receiver_rpc;
  //TODO: 보낼 데이터를 받을 때 까지 rpc 는 종료 되면 안됩니다. nullptr 이 됬다는건 데이터를 전송했다는 뜻이 됩니다.
  while(m_server.m_receiver_list[receiver_id].stream != nullptr){
	std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  return Status(StatusCode::ABORTED , "Connection aborted");
}

Status MergerRpcServer::SignerService::join (ServerContext *context, const GrpcMsgJoin *msg_join,
											 GrpcMsgChallenge *msg_challenge) {
  std::string raw_data(msg_join->message());
  uint64_t receiver_id;
  Status st = analyzeData(raw_data, receiver_id);
  if(st.ok()) {
	m_server.m_receiver_list[receiver_id].msg_challenge = msg_challenge;
	while(m_server.m_receiver_list[receiver_id].msg_challenge != nullptr){
	  std::this_thread::sleep_for(std::chrono::milliseconds(150));
	}
	return st;
  }
  return st;
}

Status MergerRpcServer::SignerService::dhKeyEx (grpc::ServerContext *context,
												const GrpcMsgResponse1 *msg_response1,
												GrpcMsgResponse2 *msg_response2) {
  std::string raw_data(msg_response1->message());
  uint64_t receiver_id;
  Status st = analyzeData(raw_data, receiver_id);
  if(st.ok()) {
	m_server.m_receiver_list[receiver_id].msg_response2 = msg_response2;
	while(m_server.m_receiver_list[receiver_id].msg_response2 != nullptr){
	  std::this_thread::sleep_for(std::chrono::milliseconds(150));
	}

	return st;
  }
  return st;
}

Status MergerRpcServer::SignerService::keyExFinished (ServerContext *context,
													  const GrpcMsgSuccess *msg_success,
													  GrpcMsgAccept *msg_accept) {
  std::string raw_data(msg_success->message());
  uint64_t receiver_id;
  Status st = analyzeData(raw_data, receiver_id);
  if(st.ok()) {
	m_server.m_receiver_list[receiver_id].msg_accept = msg_accept;
	while(m_server.m_receiver_list[receiver_id].msg_accept != nullptr){
	  std::this_thread::sleep_for(std::chrono::milliseconds(150));
	}

	return st;
  }
  return st;
}

Status MergerRpcServer::SignerService::sigSend (ServerContext *context,
												const GrpcMsgSsig *msg_ssig,
												NoReply *no_reply) {
  std::string raw_data(msg_ssig->message());
  uint64_t receiver_id;
  Status st = analyzeData(raw_data, receiver_id);
  if(st.ok()) {
	m_server.m_receiver_list[receiver_id].no_reply = no_reply;
	while(m_server.m_receiver_list[receiver_id].no_reply != nullptr){
	  std::this_thread::sleep_for(std::chrono::milliseconds(150));
	}

	return st;
  }
  return st;
}

Status MergerRpcServer::SignerService::analyzeData(std::string &raw_data, uint64_t &receiver_id){
  auto &input_queue = Application::app().getInputQueue();

  MessageHeader msg_header = HeaderController::parseHeader(raw_data);
  if(!HeaderController::validateMessage(msg_header)){
	return Status(StatusCode::INVALID_ARGUMENT , "Wrong Message");
  }
  std::string no_header_data = HeaderController::detachHeader(raw_data);
  int json_size = HeaderController::getJsonSize(msg_header);

  Message msg(msg_header);
  nlohmann::json json_data = HeaderController::getJsonMessage(msg_header.compression_algo_type, no_header_data, json_size);

  if(!JsonValidator::validateSchema(json_data, msg.message_type)) {
	return Status(StatusCode::INVALID_ARGUMENT, "json schema check fail");
  }
  uint64_t id;
  memcpy(&id, &msg.sender_id[0], sizeof(uint64_t));
  receiver_id = id;
  msg.data = json_data;
  input_queue->push(msg);

  return Status::OK;
}

void MergerRpcClient::run() {
  auto &output_queue = Application::app().getOutputQueue();
  //TODO: 아래 if문을 포함하는 무한루프 필요
  if(!output_queue->empty()) {
	Message msg = output_queue->front();
	if(checkMsgType(msg.message_type)) {
	  output_queue->pop();
	  std::string compressed_json;
	  std::string json_dump = msg.data.dump();
	  Compressor::compressData(json_dump, compressed_json);
	  std::string header_added_data = HeaderController::attachHeader(compressed_json,
																	 msg.message_type,
																	 msg.mac_algo_type,
																	 msg.compression_algo_type);

	  // TODO: MAC 붙이는 것 필요
	  sendDataToSigner(header_added_data);
	}
  }
}

bool MergerRpcClient::pushData(
	std::string &compressed_data,
	std::unique_ptr<MergerCommunication::Stub> stub) {
  MergerDataRequest request;
  request.set_data(compressed_data);

  MergerDataReply reply;
  ClientContext context;
  Status status = stub->pushData(&context, request, &reply);

  if (status.ok())
	return reply.checker();
  else {
	std::cout << status.error_code() << ": " << status.error_message()
			  << std::endl;
	return false;
  }
}

bool MergerRpcClient::checkMsgType(MessageType msg_type) {
  return (msg_type == MessageType::MSG_ECHO ||
	  msg_type == MessageType::MSG_BLOCK);
}

void MergerRpcClient::sendDataToSigner(std::string &header_added_data){
  // TODO: 현재는 로컬호스트로 받는곳 지정 해놓음 변경 필요.
  std::unique_ptr<MergerCommunication::Stub> stub =
	  MergerCommunication::NewStub(grpc::CreateChannel(
		  "localhost:50051", grpc::InsecureChannelCredentials()));

  std::thread th([&]() { pushData(header_added_data, move(stub)); });
}
} // namespace gruut