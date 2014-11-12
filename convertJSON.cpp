//============================================================================
// Name        : examples-rapidjson.cpp
// Author      : Manos Karpathiotakis
// Version     :
// Copyright   : 
// Description : Basic rapidjson usage + csv export
//============================================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstdlib>
#include <cstddef>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "rapidjson/reader.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#define DELIM ","

using namespace rapidjson;
using namespace std;

void iterateObject(Value::ConstMemberIterator& itr, Value::ConstMemberIterator& itrEnd, StringBuffer *buffer,Writer<StringBuffer> *writer, stringstream &ss);
void iterateArray(Value::ConstValueIterator& itr, Value::ConstValueIterator& itrEnd, StringBuffer *buffer,Writer<StringBuffer> *writer, stringstream &ss);
void convertMultiple(const char* nameJSON, const char* nameCSV);
void convert(const char* nameJSON, const char* nameCSV);
/**
 * Assumptions:
 * Non-empty file provided
 *
 * FIXME Does not handle correctly strings containing newline
 */
int main(int argc, char **argv) {

	if(argc < 4)	{
		printf("[USAGE: ] ./main <mode> <filenameJSON> <fileNameCSV>\n");
		printf("[Mode: 1] Input file contains a JSON object per row. Not stricty valid JSON\n");
		printf("[Mode: 2] Input file is a valid JSON element\n");
		return -1;
	}

	int mode = atoi(argv[1]);
	const char* nameJSON = argv[2];
	const char* nameCSV = argv[3];

	if(mode == 1)	{
		convertMultiple(nameJSON,nameCSV);
	}	else if(mode == 2)	{
		convert(nameJSON,nameCSV);
	}	else 	{
		throw runtime_error(string("Invalid mode"));
	}	
	return 0;
}

void convertMultiple(const char* nameJSON, const char* nameCSV)	{
	//Prepare Input
	struct stat statbuf;
	stat(nameJSON, &statbuf);
	size_t fsize = statbuf.st_size;

	int fd = open(nameJSON, O_RDONLY);
	if (fd == -1) {
		throw runtime_error(string("json.open"));
	}

	const char *bufJSON = (const char*) mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
	if (bufJSON == MAP_FAILED) {
		throw runtime_error(string("json.mmap"));
	}

	//Prepare output
	ofstream outFile;
	outFile.open(nameCSV);

	//Input loop
	size_t obj_start = 0;
	size_t obj_end = 0;
	char *line_bufJSON = NULL;
	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	stringstream ss;


	int flushCount = 0;
	while (obj_start < fsize) {
		size_t i = obj_start;
		for (; bufJSON[i] != '\n'; i++) {}
		obj_end = i;
		line_bufJSON = new char[obj_end - obj_start + 1];
		line_bufJSON[obj_end - obj_start] = '\0';
		memcpy(line_bufJSON, bufJSON + obj_start, obj_end - obj_start);

		//Triggering parser
		Document d;
		d.Parse(line_bufJSON);

		Value::ConstMemberIterator itrDoc = d.MemberBegin();
		Value::ConstMemberIterator itrDocEnd = d.MemberEnd();
		buffer.Clear();
		writer.Reset(buffer);

		//1st iteration unrolled to avoid unnecessary "if(first)" checks
		if (itrDoc != itrDocEnd) {
			if (itrDoc->value.IsObject()) {
				Value::ConstMemberIterator itr_ = itrDoc->value.MemberBegin();
				Value::ConstMemberIterator itrEnd_ = itrDoc->value.MemberEnd();
				iterateObject(itr_, itrEnd_, &buffer, &writer, ss);
			} else if (itrDoc->value.IsArray()) {
				Value::ConstValueIterator itr_ = itrDoc->value.Begin();
				Value::ConstValueIterator itrEnd_ = itrDoc->value.End();
				iterateArray(itr_, itrEnd_, &buffer, &writer, ss);
			} else if (itrDoc->value.IsBool()) {
				ss << itrDoc->value.GetBool();
			} else if (itrDoc->value.IsInt()) {
				ss << itrDoc->value.GetInt();
			} else if (itrDoc->value.IsInt64()) {
				ss << itrDoc->value.GetInt64();
			} else if (itrDoc->value.IsDouble()) {
				ss << itrDoc->value.GetDouble();
			} else if (itrDoc->value.IsString()) {
				ss << "\"" << itrDoc->value.GetString() << "\"";
			} else {
				throw runtime_error(string("Case missing from tokenizer"));
			}
		}
		itrDoc++;
		iterateObject(itrDoc, itrDocEnd, &buffer, &writer, ss);
		ss << "\n";

		//flushing to output file every 1000 entries
		flushCount++;
		if(flushCount % 1000 == 0)	{
			outFile << ss.rdbuf();
			ss.clear();
		}

		//Prepare next loop + cleanup
		delete line_bufJSON;
		obj_start = ++i;

	}

	outFile << ss.rdbuf();
	outFile.close();

	close(fd);
	munmap((void*) bufJSON,fsize);
}

void convert(const char* nameJSON, const char* nameCSV)	{
	
	//Prepare Input
	struct stat statbuf;
	stat(nameJSON, &statbuf);
	size_t fsize = statbuf.st_size;

	int fd = open(nameJSON, O_RDONLY);
	if (fd == -1) {
		throw runtime_error(string("json.open"));
	}

	const char *bufJSON = (const char*) mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
	if (bufJSON == MAP_FAILED) {
		throw runtime_error(string("json.mmap"));
	}
	//Prepare output
	ofstream outFile;
	outFile.open(nameCSV);

	Document d;
	d.Parse(bufJSON);

	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	stringstream ss;
	Value::ConstMemberIterator itrDoc = d.MemberBegin();
	Value::ConstMemberIterator itrDocEnd = d.MemberEnd();

	if (itrDoc != itrDocEnd) {
		if (itrDoc->value.IsObject()) {
			Value::ConstMemberIterator itr_ = itrDoc->value.MemberBegin();
			Value::ConstMemberIterator itrEnd_ = itrDoc->value.MemberEnd();
			iterateObject(itr_, itrEnd_, &buffer, &writer, ss);
		} else if (itrDoc->value.IsArray()) {
			Value::ConstValueIterator itr_ = itrDoc->value.Begin();
			Value::ConstValueIterator itrEnd_ = itrDoc->value.End();
			iterateArray(itr_, itrEnd_, &buffer, &writer, ss);
		} else if (itrDoc->value.IsBool()) {
			ss << itrDoc->value.GetBool();
		} else if (itrDoc->value.IsInt()) {
			ss << itrDoc->value.GetInt();
		} else if (itrDoc->value.IsInt64()) {
			ss << itrDoc->value.GetInt64();
		} else if (itrDoc->value.IsDouble()) {
			ss << itrDoc->value.GetDouble();
		} else if (itrDoc->value.IsString()) {
			ss << "\"" << itrDoc->value.GetString() << "\"";
		} else {
			throw runtime_error(string("Case missing from tokenizer"));
		}
	}
	itrDoc++;
	iterateObject(itrDoc, itrDocEnd, &buffer, &writer, ss);

	outFile << ss.rdbuf();
	outFile.close();

	close(fd);
	munmap((void*) bufJSON,fsize);
}

void iterateArray(Value::ConstValueIterator& itr,
		Value::ConstValueIterator& itrEnd, StringBuffer *buffer,
		Writer<StringBuffer> *writer, stringstream& ss) {
	for (; itr != itrEnd; itr++) {
		if (itr->IsObject()) {
			Value::ConstMemberIterator itr_ = itr->MemberBegin();
			Value::ConstMemberIterator itrEnd_ = itr->MemberEnd();
			iterateObject(itr_, itrEnd_, buffer, writer, ss);
		} else if (itr->IsArray()) {
			Value::ConstValueIterator itr_ = itr->Begin();
			Value::ConstValueIterator itrEnd_ = itr->End();
			iterateArray(itr_, itrEnd_, buffer, writer, ss);
		} else if (itr->IsBool()) {
			ss << DELIM << itr->GetBool();
		} else if (itr->IsInt()) {
			ss << DELIM << itr->GetInt();
		} else if (itr->IsInt64()) {
			ss << DELIM << itr->GetInt64();
		} else if (itr->IsDouble()) {
			ss << DELIM << itr->GetDouble();
		} else if (itr->IsString()) {
			ss << DELIM << "\"" << itr->GetString() << "\"";
		} else {
			throw runtime_error(string("Case missing from tokenizer"));
		}
	}
}

void iterateObject(Value::ConstMemberIterator& itr,
		Value::ConstMemberIterator& itrEnd, StringBuffer *buffer,
		Writer<StringBuffer> *writer, stringstream& ss) {
	for (; itr != itrEnd; itr++) {
		if (itr->value.IsObject()) {
			Value::ConstMemberIterator itr_ = itr->value.MemberBegin();
			Value::ConstMemberIterator itrEnd_ = itr->value.MemberEnd();
			iterateObject(itr_, itrEnd_, buffer, writer, ss);
		} else if (itr->value.IsArray()) {
			Value::ConstValueIterator itr_ = itr->value.Begin();
			Value::ConstValueIterator itrEnd_ = itr->value.End();
			iterateArray(itr_, itrEnd_, buffer, writer, ss);
		} else if (itr->value.IsBool()) {
			ss << DELIM << itr->value.GetBool();
		} else if (itr->value.IsInt()) {
			ss << DELIM << itr->value.GetInt();
		} else if (itr->value.IsInt64()) {
			ss << DELIM << itr->value.GetInt64();
		} else if (itr->value.IsDouble()) {
			ss << DELIM << itr->value.GetDouble();
		} else if (itr->value.IsString()) {
			ss << DELIM << "\"" << itr->value.GetString() << "\"";
		} else {
			std::cout << "Forgot some case" << endl;
			return;
		}
	}
}
