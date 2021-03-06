#include <glog/logging.h>
#include <chrono>

#include <net/mpi/mpi_communicator.hpp>
#include <ctx/cylon_context.hpp>
#include <table.hpp>
#include <ctx/arrow_memory_pool_utils.hpp>
#include <map>

#include "indexing/index_utils.hpp"
#include "indexing/indexer.hpp"
#include "test_utils.hpp"

namespace cylon{
namespace test{
int TestIndexBuildOperation(std::string &input_file_path, cylon::IndexingSchema indexing_schema) {
  auto mpi_config = std::make_shared<cylon::net::MPIConfig>();
  auto ctx = cylon::CylonContext::InitDistributed(mpi_config);

  cylon::Status status;

  std::shared_ptr<cylon::Table> input1, output;
  std::shared_ptr<cylon::BaseIndex> index;
  auto read_options = cylon::io::config::CSVReadOptions().UseThreads(false).BlockSize(1 << 30);

  // read first table
  std::cout << "Reading File [" << ctx->GetRank() << "] : " << input_file_path << std::endl;
  status = cylon::FromCSV(ctx, input_file_path, input1, read_options);

  const int index_column = 0;

  status = cylon::IndexUtil::BuildIndex(indexing_schema, input1, index_column, index);

  if (!status.is_ok()) {
    return 1;
  }

  bool valid_index;

  valid_index = input1->Rows() == index->GetIndexArray()->length();

  if (!valid_index) {
    return 1;
  };

  return 0;
}

static int set_data_for_indexing_test(std::string &input_file_path,
                                      cylon::IndexingSchema indexing_schema,
                                      std::string &output_file_path,
                                      std::shared_ptr<cylon::Table> &input,
                                      std::shared_ptr<cylon::Table> &expected_output,
                                      std::shared_ptr<cylon::BaseIndex> &index,
                                      int id
) {

  auto mpi_config = std::make_shared<cylon::net::MPIConfig>();
  auto ctx = cylon::CylonContext::InitDistributed(mpi_config);

  cylon::Status status;
  output_file_path = output_file_path + std::to_string(id) + ".csv";
  auto read_options = cylon::io::config::CSVReadOptions().UseThreads(false).BlockSize(1 << 30);

  // read first table
  std::cout << "Reading File [" << ctx->GetRank() << "] : " << input_file_path << std::endl;
  status = cylon::FromCSV(ctx, input_file_path, input, read_options);

  if(!status.is_ok()) {
    return 1;
  }

  status = cylon::FromCSV(ctx, output_file_path, expected_output, read_options);

  if(!status.is_ok()) {
    return 1;
  }

  const int index_column = 0;
  bool drop_index = true;

  status = cylon::IndexUtil::BuildIndex(indexing_schema, input, index_column, index);

  if (!status.is_ok()) {
    return 1;
  }

  bool valid_index = false;
  if (indexing_schema == cylon::IndexingSchema::Range) {
    valid_index = input->Rows() == index->GetSize();
  } else {
    valid_index = input->Rows() == index->GetIndexArray()->length();
  }

  if (!valid_index) {
    return 1;
  };

  status = input->Set_Index(index, drop_index);

  if (!status.is_ok()) {
    return 1;
  }

  return 0;
}


int TestIndexLocOperation1(std::string &input_file_path,
                           cylon::IndexingSchema indexing_schema,
                           std::string &output_file_path) {
  auto mpi_config = std::make_shared<cylon::net::MPIConfig>();
  auto ctx = cylon::CylonContext::InitDistributed(mpi_config);

  cylon::Status status;
  std::shared_ptr<cylon::Table> input, expected_output, result;
  std::shared_ptr<BaseIndex> index;
  int res = set_data_for_indexing_test(input_file_path, indexing_schema, output_file_path, input, expected_output, index, 1);

  if(res != 0) {
    return res;
  }

  long start_index = 0;
  long end_index = 5;
  int column = 0;

  std::shared_ptr<cylon::BaseIndexer> indexer = std::make_shared<cylon::LocIndexer>(indexing_schema);

  status = indexer->loc(&start_index, &end_index, column, input, result);

  if (!status.is_ok()) {
    return -1;
  }

  //auto write_options = io::config::CSVWriteOptions();
  //cylon::WriteCSV(result, "/tmp/indexing_result_check" + std::to_string(indexing_schema) + ".csv", write_options);

  if (test::Verify(ctx, result, expected_output)) {
    LOG(ERROR) << "Loc 1 failed!";
    return 1;
  }

  return 0;
}



int TestIndexLocOperation2(std::string &input_file_path,
                           cylon::IndexingSchema indexing_schema,
                           std::string &output_file_path) {
  auto mpi_config = std::make_shared<cylon::net::MPIConfig>();
  auto ctx = cylon::CylonContext::InitDistributed(mpi_config);

  cylon::Status status;

  std::shared_ptr<cylon::Table> input, expected_output, result;
  std::shared_ptr<cylon::BaseIndex> index;
  int res = set_data_for_indexing_test(input_file_path, indexing_schema, output_file_path, input, expected_output, index, 2);

  if(res != 0) {
    return res;
  }

  long start_index = 0;
  long end_index = 5;
  int start_column_index = 0;
  int end_column_index = 1;

  std::shared_ptr<cylon::BaseIndexer> indexer = std::make_shared<cylon::LocIndexer>(indexing_schema);

  status = indexer->loc(&start_index, &end_index, start_column_index, end_column_index, input, result);

  if (!status.is_ok()) {
    return -1;
  }

  if (test::Verify(ctx, result, expected_output)) {
    LOG(ERROR) << "Loc 2 failed!";
    return 1;
  }

  return 0;
}


int TestIndexLocOperation3(std::string &input_file_path,
                           cylon::IndexingSchema indexing_schema,
                           std::string &output_file_path) {
  auto mpi_config = std::make_shared<cylon::net::MPIConfig>();
  auto ctx = cylon::CylonContext::InitDistributed(mpi_config);

  cylon::Status status;

  std::shared_ptr<cylon::Table> input, expected_output, result;
  std::shared_ptr<cylon::BaseIndex> index;
  int res = set_data_for_indexing_test(input_file_path, indexing_schema, output_file_path, input, expected_output, index, 3);

  if(res != 0) {
    return res;
  }

  long start_index = 0;
  long end_index = 5;
  std::vector<int> cols = {0,2};

  std::shared_ptr<cylon::BaseIndexer> indexer = std::make_shared<cylon::LocIndexer>(indexing_schema);

  status = indexer->loc(&start_index, &end_index, cols, input, result);

  if (!status.is_ok()) {
    return -1;
  }

  if (test::Verify(ctx, result, expected_output)) {
    LOG(ERROR) << "Loc 3 failed!";
    return 1;
  }

  return 0;
}

int TestIndexLocOperation4(std::string &input_file_path,
                           cylon::IndexingSchema indexing_schema,
                           std::string &output_file_path) {
  auto mpi_config = std::make_shared<cylon::net::MPIConfig>();
  auto ctx = cylon::CylonContext::InitDistributed(mpi_config);

  cylon::Status status;

  std::shared_ptr<cylon::Table> input, expected_output, result;
  std::shared_ptr<cylon::BaseIndex> index;
  int res = set_data_for_indexing_test(input_file_path, indexing_schema, output_file_path, input, expected_output, index, 4);

  if(res != 0) {
    return res;
  }

  long start_index = 10;
  int column = 1;

  std::shared_ptr<cylon::BaseIndexer> indexer = std::make_shared<cylon::LocIndexer>(indexing_schema);

  status = indexer->loc(&start_index, column, input, result);

  if (!status.is_ok()) {
    return -1;
  }

  if (test::Verify(ctx, result, expected_output)) {
    LOG(ERROR) << "Loc 4 failed!";
    return 1;
  }

  return 0;
}


int TestIndexLocOperation5(std::string &input_file_path,
                           cylon::IndexingSchema indexing_schema,
                           std::string &output_file_path) {
  auto mpi_config = std::make_shared<cylon::net::MPIConfig>();
  auto ctx = cylon::CylonContext::InitDistributed(mpi_config);

  cylon::Status status;

  std::shared_ptr<cylon::Table> input, expected_output, result;
  std::shared_ptr<cylon::BaseIndex> index;
  int res = set_data_for_indexing_test(input_file_path, indexing_schema, output_file_path, input, expected_output, index, 5);

  if(res != 0) {
    return res;
  }

  typedef std::vector<void *> vector_void_star;

  vector_void_star output_items;

  std::vector<long> start_indices = {4, 10};
  int start_column = 1;
  int end_column = 2;

  for (size_t tx = 0; tx < start_indices.size(); tx++) {
    long *val = new long(start_indices.at(tx));
    output_items.push_back(static_cast<void *>(val));
  }

  std::shared_ptr<cylon::BaseIndexer> indexer = std::make_shared<cylon::LocIndexer>(indexing_schema);

  status = indexer->loc(output_items, start_column, end_column, input, result);

  if (!status.is_ok()) {
    return -1;
  }

  if (test::Verify(ctx, result, expected_output)) {
    LOG(ERROR) << "Loc 5 failed!";
    return 1;
  }

  return 0;
}

int TestIndexLocOperation6(std::string &input_file_path,
                           cylon::IndexingSchema indexing_schema,
                           std::string &output_file_path) {
  auto mpi_config = std::make_shared<cylon::net::MPIConfig>();
  auto ctx = cylon::CylonContext::InitDistributed(mpi_config);

  cylon::Status status;

  std::shared_ptr<cylon::Table> input, expected_output, result;
  std::shared_ptr<cylon::BaseIndex> index;
  int res = set_data_for_indexing_test(input_file_path, indexing_schema, output_file_path, input, expected_output, index, 6);

  if(res != 0) {
    return res;
  }

  typedef std::vector<void *> vector_void_star;

  vector_void_star output_items;

  std::vector<long> start_indices = {4, 10};
  std::vector<int> columns = {0, 2};

  for (size_t tx = 0; tx < start_indices.size(); tx++) {
    long *val = new long(start_indices.at(tx));
    output_items.push_back(static_cast<void *>(val));
  }

  std::shared_ptr<cylon::BaseIndexer> indexer = std::make_shared<cylon::LocIndexer>(indexing_schema);

  status = indexer->loc(output_items, columns, input, result);

  if (!status.is_ok()) {
    return -1;
  }

  if (test::Verify(ctx, result, expected_output)) {
    LOG(ERROR) << "Loc 6 failed!";
    return 1;
  }

  return 0;
}

int TestIndexLocOperation7(std::string &input_file_path,
                           cylon::IndexingSchema indexing_schema,
                           std::string &output_file_path) {
  auto mpi_config = std::make_shared<cylon::net::MPIConfig>();
  auto ctx = cylon::CylonContext::InitDistributed(mpi_config);

  cylon::Status status;

  std::shared_ptr<cylon::Table> input, expected_output, result;
  std::shared_ptr<cylon::BaseIndex> index;
  int res = set_data_for_indexing_test(input_file_path, indexing_schema, output_file_path, input, expected_output, index, 7);

  if(res != 0) {
    return res;
  }


  long start_index = 4;
  std::vector<int> columns = {0, 1};

  std::shared_ptr<cylon::BaseIndexer> indexer = std::make_shared<cylon::LocIndexer>(indexing_schema);

  status = indexer->loc(&start_index, columns, input, result);

  if (!status.is_ok()) {
    return -1;
  }

  if (test::Verify(ctx, result, expected_output)) {
    LOG(ERROR) << "Loc 7 failed!";
    return 1;
  }

  return 0;
}

int TestIndexLocOperation8(std::string &input_file_path,
                           cylon::IndexingSchema indexing_schema,
                           std::string &output_file_path) {
  auto mpi_config = std::make_shared<cylon::net::MPIConfig>();
  auto ctx = cylon::CylonContext::InitDistributed(mpi_config);

  cylon::Status status;

  std::shared_ptr<cylon::Table> input, expected_output, result;
  std::shared_ptr<cylon::BaseIndex> index;
  int res = set_data_for_indexing_test(input_file_path, indexing_schema, output_file_path, input, expected_output, index, 8);

  if(res != 0) {
    return res;
  }


  long start_index = 4;
  int start_column = 1;
  int end_column = 2;

  std::shared_ptr<cylon::BaseIndexer> indexer = std::make_shared<cylon::LocIndexer>(indexing_schema);

  status = indexer->loc(&start_index, start_column, end_column, input, result);

  if (!status.is_ok()) {
    return -1;
  }

  if (test::Verify(ctx, result, expected_output)) {
    LOG(ERROR) << "Loc 8 failed!";
    return 1;
  }

  return 0;
}

int TestIndexLocOperation9(std::string &input_file_path,
                           cylon::IndexingSchema indexing_schema,
                           std::string &output_file_path) {
  auto mpi_config = std::make_shared<cylon::net::MPIConfig>();
  auto ctx = cylon::CylonContext::InitDistributed(mpi_config);

  cylon::Status status;

  std::shared_ptr<cylon::Table> input, expected_output, result;
  std::shared_ptr<cylon::BaseIndex> index;
  int res = set_data_for_indexing_test(input_file_path, indexing_schema, output_file_path, input, expected_output, index, 9);

  if(res != 0) {
    return res;
  }


  typedef std::vector<void *> vector_void_star;

  vector_void_star output_items;

  std::vector<long> start_indices = {4, 10};
  int column = 0;

  for (size_t tx = 0; tx < start_indices.size(); tx++) {
    long *val = new long(start_indices.at(tx));
    output_items.push_back(static_cast<void *>(val));
  }

  std::shared_ptr<cylon::BaseIndexer> indexer = std::make_shared<cylon::LocIndexer>(indexing_schema);

  status = indexer->loc(output_items, column, input, result);

  if (!status.is_ok()) {
    return -1;
  }

  if (test::Verify(ctx, result, expected_output)) {
    LOG(ERROR) << "Loc 9 failed!";
    return 1;
  }

  return 0;
}

}
}
