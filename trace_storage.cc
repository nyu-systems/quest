// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "google/cloud/storage/client.h"
#include "opentelemetry/proto/trace/v1/trace.pb.h"
#include <iostream>
#include <regex>
#include <boost/regex.hpp>
#include <boost/algorithm/string/regex.hpp>



const std::string trace_struct_bucket = "dyntraces-snicket3";
const std::string ending = "-snicket3";
// Create aliases to make the code easier to read.
namespace gcs = ::google::cloud::storage;

std::vector<std::string> split_string_by_newline(const std::string& str)
{
    std::vector<std::string> tokens;
    split_regex(tokens, str, boost::regex("(\n)+"));
    return tokens;
}

std::vector<std::string> split_string_by_colon(const std::string& str)
{
    std::vector<std::string> tokens;
    split_regex(tokens, str, boost::regex("(:)+"));
    return tokens;
}


std::string get_span(int hash1, int hash2, std::string microservice, std::string start_time, std::string end_time, gcs::Client* client) {
    std::string obj_name = std::to_string(hash1) + std::to_string(hash2) + "-" + start_time + "-" + end_time;
    auto reader = client->ReadObject(microservice+ending, obj_name);
    if (reader.status().code() == ::google::cloud::StatusCode::kNotFound) {
        std::cerr << "span object not found " << obj_name << "in microservice " << microservice << std::endl;
    } else if (!reader) {
        std::cerr << "Error reading object: " << reader.status() << "\n";
        // there's gotta be a better thing to return here
        return "";
    } else {
        std::string contents{std::istreambuf_iterator<char>{reader}, {}};
        opentelemetry::proto::trace::v1::TracesData trace_data;
        bool ret = trace_data.ParseFromString(contents);
        if (ret) {
        std::cout << "not parsed" << std::endl;
        } else {
            std::cout << " parsed! " << std::endl;
        }
/*
        if (opentelemetry::proto::trace::v1::ResourceSpans::ParseFromString(contents)) {
            std::cout << "parsed!" << std::endl;
        } else {
            std::cout << "oh no" << std::endl;
        }
        */
        return contents;
    }
    return "";
}

// Gets a trace by trace ID and given timespan
int get_trace(std::string traceID, int start_time, int end_time, gcs::Client* client) {
    bool trace_found = false;
    for (int i=0; i<10; i++) {
        if (trace_found) {
            break;
        }
        for (int j=0; j<10; j++) {
          if (trace_found) {
            break;
          }
          std::string obj_name = std::to_string(i) + std::to_string(j) + "-";
          obj_name += std::to_string(start_time) + "-" + std::to_string(end_time);
          auto reader = client->ReadObject(trace_struct_bucket, obj_name);
          if (reader.status().code() == ::google::cloud::StatusCode::kNotFound) {
            continue;
          } else if (!reader) {
            std::cerr << "Error reading object: " << reader.status() << "\n";
            return 1;
          } else {
            std::string contents{std::istreambuf_iterator<char>{reader}, {}};
            int traceID_location = contents.find(traceID);
            if (traceID_location) {
                trace_found = true;
                int end = contents.find("Trace ID", traceID_location-1);
                if (end) {
                    std::string spans = contents.substr(traceID_location, end-traceID_location);
                    std::cout << spans << std::endl;
                    std::vector<std::string> split_spans = split_string_by_newline(spans);
                    // start at 1 because first line will be trace ID
                    for (int k = 1; k < split_spans.size(); k++) {
                        std::cout << "token " << split_spans[k] << std::endl;
                        if (split_spans[k] != "") {
                            std::vector<std::string> span_info = split_string_by_colon(split_spans[k]);
                            std::cout << "span info 2 is " << span_info[2] << std::endl;
                            std::string span = get_span(i, j, span_info[2], std::to_string(start_time), std::to_string(end_time), client);
                            //std::cout << "span " << span << std::endl << std::endl;
                        }

                    }


                } else { std::cout << "couldn't find Trace ID" << std::endl; }
            }
            
          }
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Missing bucket name.\n";
    std::cerr << "Usage: quickstart <bucket-name>\n";
    return 1;
  }
  std::string const bucket_name = argv[1];


  // Create a client to communicate with Google Cloud Storage. This client
  // uses the default configuration for authentication and project id.
  auto client = gcs::Client();

  auto writer = client.WriteObject(bucket_name, "quickstart.txt");
  writer << "Hello World!";
  writer.Close();
  if (writer.metadata()) {
    std::cout << "Successfully created object: " << *writer.metadata() << "\n";
  } else {
    std::cerr << "Error creating object: " << writer.metadata().status()
              << "\n";
    return 1;
  }

  auto reader = client.ReadObject(bucket_name, "quickstart.txt");
  if (!reader) {
    std::cerr << "Error reading object: " << reader.status() << "\n";
    return 1;
  }

  std::string contents{std::istreambuf_iterator<char>{reader}, {}};
  std::cout << contents << "\n";

  get_trace("366ada8fbc705fbddf0468d1df1e746f", 1651073970, 1651073970, &client);

  return 0;
}
