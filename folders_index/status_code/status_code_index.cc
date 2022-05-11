// Copyright 2022 Haseeb LLC
// @author: Muhammad Haseeb <mh6218@nyu.edu>

#include "status_code_index.h"

int main(int argc, char* argv[]) {
	dummy_tests();

	auto client = gcs::Client();
	time_t last_updated = 0;
	trace_attribute indexed_attribute = HTTP_STATUS_CODE;
	std::string attribute_value = "202";

	return update_index(&client, last_updated, indexed_attribute, attribute_value);
}

int update_index(gcs::Client* client, time_t last_updated, 
	trace_attribute indexed_attribute, std::string attribute_value
) {
	std::vector<std::string> span_buckets_names = get_spans_buckets_names(client);
	/**
	 * TODO: (i) Following is a bad thing to do. what if all object names do not fit in the memory. 
	 * (ii) do the error handling for the case when bucket is not present. 
	 */
	std::vector<std::string> trace_struct_object_names = get_all_object_names(TRACE_STRUCT_BUCKET, client);
	trace_struct_object_names = sort_object_names_on_start_time(trace_struct_object_names);
	index_batch current_index_batch = index_batch();

	for (auto object_name : trace_struct_object_names) {
		std::vector<std::string> trace_ids_with_attribute = get_trace_ids_with_attribute(
			object_name, indexed_attribute, attribute_value, span_buckets_names, client);

		current_index_batch.total_trace_ids += trace_ids_with_attribute.size();
		current_index_batch.trace_ids_with_timestamps.push_back(std::make_pair(
			extract_batch_timestamps(object_name), trace_ids_with_attribute
		));

		if (true == is_batch_big_enough(current_index_batch)) {
			auto status = export_batch_to_storage(current_index_batch, attribute_value);
			if (status != 0) {
				std::cout << "Could not export an index batch to Cloud Storage!" << std::endl;
				exit(1);
			}

			current_index_batch = index_batch();
			break;  // TODO remove it
		}
	}
	return 0;
}

std::vector<std::string> get_spans_buckets_names(gcs::Client* client) {
	std::vector<std::string> response;

	for (auto&& bucket_metadata : client->ListBucketsForProject(PROJECT_ID)) {
		if (!bucket_metadata) {
			std::cerr << bucket_metadata.status().message() << std::endl;
			exit(1);
		}

		if (true == bucket_metadata->labels().empty()) {
			continue;
		}

		const auto label_map = bucket_metadata->labels();
		if (label_map.at(BUCKET_TYPE_LABEL_KEY) == BUCKET_TYPE_LABEL_VALUE_FOR_SPAN_BUCKETS) {
			response.push_back(bucket_metadata->name());
		}
	}

	return response;
}

std::vector<std::string> get_all_object_names(std::string bucket_name, gcs::Client* client) {
	std::vector<std::string> response;

	for (auto&& object_metadata : client->ListObjects(bucket_name)) {
		if (!object_metadata) {
			std::cerr << object_metadata.status().message() << std::endl;
			exit(1);
		}

		response.push_back(object_metadata->name());
	}

	return response;
}

std::vector<std::string> split_by_char(std::string input, std::string splitter) {
	std::vector<std::string> result;
	boost::split(result, input, boost::is_any_of(splitter));
	return result;
}


bool compare_object_names_by_start_time(std::string object_name1, std::string object_name2) {
	// Object name format is somehash-starttime-endtime
	return std::stol(split_by_char(object_name1, "-")[1]) < std::stol(split_by_char(object_name2, "-")[1]);
}

std::vector<std::string> sort_object_names_on_start_time(std::vector<std::string> object_names) {
	sort(object_names.begin(), object_names.end(), compare_object_names_by_start_time);
	return object_names;	
}

std::vector<std::string> get_trace_ids_with_attribute(
	std::string object_name, trace_attribute indexed_attribute, std::string attribute_value,
	std::vector<std::string>& span_buckets_names, gcs::Client* client
) {
	for (auto span_bucket : span_buckets_names) {
		std::unordered_map<std::string, bool> trace_id_to_attribute_membership = calculate_trace_id_to_attribute_map(
			span_bucket, object_name, indexed_attribute, attribute_value, client);
	}

	// HERE
}

std::unordered_map<std::string, bool> calculate_trace_id_to_attribute_map(std::string span_bucket_name,
	std::string object_name, trace_attribute indexed_attribute, std::string attribute_value, gcs::Client* client
) {
	std::unordered_map<std::string, bool> response;
	return response;
}

batch_timestamp extract_batch_timestamps(std::string batch_name) {
	std::vector<std::string> result;
	boost::split(result, batch_name, boost::is_any_of("-"));
	if (result.size() != 3) {
		std::cerr << "Error in extract_batch_timestamps with batch name: " << batch_name << std::endl;
	}

	batch_timestamp timestamp = {result[1], result[2]};
	return timestamp;
}

bool is_batch_big_enough(index_batch& current_index_batch) {
	// TODO
	return true;
}

int export_batch_to_storage(index_batch& current_index_batch, std::string index_status_code) {
	// TODO
	return 0;
}

std::string read_object(std::string bucket, std::string object, gcs::Client* client) {
	auto reader = client->ReadObject(bucket, object);
	if (!reader) {
		std::cerr << "Error reading object " << bucket << "/" << object << " :" << reader.status() << "\n";
		exit(1);
	}

	std::string object_content{std::istreambuf_iterator<char>{reader}, {}};
	return object_content;
}

std::vector<std::string> split_by_string(std::string input, std::string splitter) {
	std::vector<std::string> result;

	size_t pos = 0;
	std::string token;
	while ((pos = input.find(splitter)) != std::string::npos) {
		token = input.substr(0, pos);
		token = strip_from_the_end(token, '\n');
		if (token.length() > 0) {
			result.push_back(token);
		}
		input.erase(0, pos + splitter.length());
	}

	input = strip_from_the_end(input, '\n');
	if (input.length() > 0) {
		result.push_back(input);
	}

	return result;
}

std::string strip_from_the_end(std::string object, char stripper) {
	if (!object.empty() && object[object.length()-1] == stripper) {
		object.erase(object.length()-1);
	}
	return object;
}

int dummy_tests() {

	// exit(1);
	return 0;
}