// main.cpp

#include <aws/core/Aws.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/platform/Environment.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/platform/Environment.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/utils/Outcome.h> 
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/AttributeDefinition.h>
#include <aws/dynamodb/model/PutRequest.h>
#include <aws/dynamodb/model/BatchWriteItemRequest.h>
#include <aws/dynamodb/model/BatchWriteItemResult.h>
#include <aws/dynamodb/model/WriteRequest.h>
#include <aws/core/utils/Array.h>
#include <aws/lambda-runtime/runtime.h>
#include <iostream>
#include <chrono>
#include <vector>
#include <memory>
#include <seal/seal.h>
#include "base64.h"

using namespace std;
using namespace std::chrono;
using namespace aws::lambda_runtime;
using namespace seal;
using namespace Aws::Utils::Json;
using namespace Aws::DynamoDB;
using namespace Aws::DynamoDB::Model;
using namespace Aws;

char const TAG[] = "VELE_LOG";

invocation_response my_handler(invocation_request const& request, DynamoDBClient const& client)
{
   AWS_LOGSTREAM_DEBUG(TAG, "received payload: " << request.payload);
   
   JsonValue json(request.payload);
   if (!json.WasParseSuccessful()) {
       return invocation_response::failure("Failed to parse input JSON", "InvalidJSON");
   }

   auto v = json.View();

   if (!v.ValueExists("params") || !v.ValueExists("key") || !v.ValueExists("latitude") || !v.ValueExists("longitude") || !v.GetObject("params").IsString() || !v.GetObject("key").IsString() || !v.GetObject("latitude").IsString() || !v.GetObject("longitude").IsString()) {
       return invocation_response::failure("Missing input value params, key, or data", "InvalidJSON");
   }

   auto paramsEnc = v.GetString("params");
   auto keyEnc = v.GetString("key");
   auto latEnc = v.GetString("latitude");
   auto longEnc = v.GetString("longitude");

   string tableParams = "vele_thesis_location_params";
   string tableKey = "vele_thesis_location_key";
   string tableLat = "vele_thesis_location_latitude";
   string tableLong = "vele_thesis_location_longitude";

   // create and setup put requests for adding items to tables
   Aws::DynamoDB::Model::PutRequest prParams;
   Aws::DynamoDB::Model::PutRequest prKey;
   Aws::DynamoDB::Model::PutRequest prLat;
   Aws::DynamoDB::Model::PutRequest prLong;

   // create id that will be shared amongst tables
   milliseconds ms = duration_cast< milliseconds >(
    system_clock::now().time_since_epoch()
   );
   signed int id = ms.count(); 
   Aws::DynamoDB::Model::AttributeValue avId;
   avId.SetN(id);

   // add id to tables
   prParams.AddItem("id", avId);
   prKey.AddItem("id", avId);
   prLat.AddItem("id", avId);
   prLong.AddItem("id", avId);

   // add param, key, lat, long values to respective tables   

   Aws::DynamoDB::Model::BatchWriteItemRequest bwir;

   Aws::DynamoDB::Model::AttributeValue avParams;
   avParams.SetS(paramsEnc);
   prParams.AddItem("params", avParams);
   Aws::DynamoDB::Model::WriteRequest wrParams;
   wrParams.SetPutRequest(prParams);
   vector<WriteRequest> tableParamsWRVect{wrParams};
   bwir.AddRequestItems(tableParams, tableParamsWRVect);

   Aws::DynamoDB::Model::AttributeValue avKey;
   avKey.SetS(keyEnc);
   prKey.AddItem("key", avKey);
   Aws::DynamoDB::Model::WriteRequest wrKey;
   wrKey.SetPutRequest(prKey);
   vector<WriteRequest> tableKeyWRVect{wrKey};
   bwir.AddRequestItems(tableKey, tableKeyWRVect);

   Aws::DynamoDB::Model::AttributeValue avLat;
   avLat.SetS(latEnc);
   prLat.AddItem("latitude", avLat);
   Aws::DynamoDB::Model::WriteRequest wrLat;
   wrLat.SetPutRequest(prLat);
   vector<WriteRequest> tableLatWRVect{wrLat};
   bwir.AddRequestItems(tableLat, tableLatWRVect);

   Aws::DynamoDB::Model::AttributeValue avLong;
   avLong.SetS(longEnc);
   prLong.AddItem("longitude", avLong);
   Aws::DynamoDB::Model::WriteRequest wrLong;
   wrLong.SetPutRequest(prLong);
   vector<WriteRequest> tableLongWRVect{wrLong};
   bwir.AddRequestItems(tableLong, tableLongWRVect);
   
   Aws::Utils::Outcome dbRes = client.BatchWriteItem(bwir);
   if (!dbRes.IsSuccess())
   {
       std::cout << dbRes.GetError().GetMessage() << std::endl;
       return invocation_response::failure("DB input did not succeed.", "InvalidQuery");
   }
   std::cout << "Done!" << std::endl;
  
   string paramsDec = base64_decode(paramsEnc);
   string keyDec = base64_decode(keyEnc);
   string encrLongDec = base64_decode(longEnc);
   string encrLatDec = base64_decode(latEnc);  
 
   stringstream paramsIn(paramsDec);
   stringstream keyIn(keyDec);
   stringstream encrLatIn(encrLatDec);
   stringstream encrLongIn(encrLongDec);

   EncryptionParameters parms(scheme_type::CKKS);
   SecretKey key;
   Plaintext x_plain;
   Ciphertext x_encrypted;
   vector<double> resultLat;
   vector<double> resultLong;

   try {
       parms.load(paramsIn);
       auto context = SEALContext::Create(parms);
       key.load(context, keyIn);
       Evaluator evaluator(context);
       Decryptor decryptor(context, key);
       CKKSEncoder encoder(context);
       x_encrypted.load(context, encrLatIn);
       decryptor.decrypt(x_encrypted, x_plain);
       encoder.decode(x_plain, resultLat);
       x_encrypted.load(context, encrLongIn);
       decryptor.decrypt(x_encrypted, x_plain);
       encoder.decode(x_plain, resultLong);
   }
   catch (std::exception& e) {
       cerr << e.what() << endl;
   }

   JsonValue output;
   Aws::Utils::Array<string> decryptedValue(2);
   
   decryptedValue[0] = to_string(resultLat.at(0));
   decryptedValue[1] = to_string(resultLong.at(0));   

   output.WithArray("decrypted", decryptedValue);

   return invocation_response::success(output.View().WriteCompact(), "application/json");
}

std::function<std::shared_ptr<Aws::Utils::Logging::LogSystemInterface>()> GetConsoleLoggerFactory()
{
    return [] {
        return Aws::MakeShared<Aws::Utils::Logging::ConsoleLogSystem>(
            "console_logger", Aws::Utils::Logging::LogLevel::Trace);
    };
}

int main()
{
   SDKOptions options;
   options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Trace;
   options.loggingOptions.logger_create_fn = GetConsoleLoggerFactory();
   InitAPI(options);
   {
       Aws::Client::ClientConfiguration config;
       config.region = Aws::Environment::GetEnv("AWS_REGION");
       config.caFile = "/etc/pki/tls/certs/ca-bundle.crt";
       config.disableExpectHeader = true;

       auto credentialsProvider = MakeShared<Aws::Auth::EnvironmentAWSCredentialsProvider>(TAG);
       DynamoDBClient client(credentialsProvider, config);
       auto handler_fn = [&client](invocation_request const& req) {
           return my_handler(req, client);
       };
       run_handler(handler_fn);
   }
   ShutdownAPI(options);
   return 0;
}
