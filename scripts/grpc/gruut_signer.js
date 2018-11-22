var PROTO_PATH = __dirname + '/protobuf_signer.proto';

var grpc = require('grpc');
var protoLoader = require('@grpc/proto-loader');
var ByteBuffer = require('bytebuffer');
var packageDefinition = protoLoader.loadSync(
    PROTO_PATH,
    {
      keepCase: true,
      longs: String,
      enums: String,
      defaults: true,
      oneofs: true
    });
let grpc_signer = grpc.loadPackageDefinition(packageDefinition).grpc_signer;

function main() {
  const client = new grpc_signer.GruutNetworkService('localhost:50051',
      grpc.credentials.createInsecure());

  var bytes = new ByteBuffer.fromHex('40414243');
  client.join({message: bytes.toBuffer()}, function (err, response) {
    if (err)
      throw err;
    console.log(response.message);
  });
}

main();
