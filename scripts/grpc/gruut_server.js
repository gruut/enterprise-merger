const PROTO_PATH = __dirname + '/protobuf_signer.proto';

var ByteBuffer = require('bytebuffer');
const grpc = require('grpc');
const protoLoader = require('@grpc/proto-loader');
const packageDefinition = protoLoader.loadSync(
    PROTO_PATH,
    {keepCase: true,
      longs: String,
      enums: String,
      defaults: true,
      oneofs: true
    });
const signer_proto = grpc.loadPackageDefinition(packageDefinition).grpc_signer;

function join(call, callback) {
  var bytes = new ByteBuffer.fromHex('40414243');
  callback(null, {message: bytes.toBuffer()});
}

/**
 * Starts an RPC server that receives requests for the Greeter service at the
 * sample server port
 */
function main() {
  let server = new grpc.Server();
  server.addService(signer_proto.GruutNetworkService.service, {join: join});
  server.bind('0.0.0.0:50051', grpc.ServerCredentials.createInsecure());
  server.start();
}

main();
