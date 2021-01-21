/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <vector>
#include <util/macros.hpp>

#include "ctx/arrow_memory_pool_utils.hpp"
#include "partition.hpp"
/*

#include <ctx/arrow_memory_pool_utils.hpp>
#include <arrow/arrow_partition_kernels.hpp>
#include "partition.hpp"

namespace cylon {
namespace kernel {
Status HashPartition(std::shared_ptr<CylonContext> &ctx, const std::shared_ptr<Table> &table,
                     const std::vector<int> &hash_columns, int no_of_partitions,
                     std::unordered_map<int, std::shared_ptr<cylon::Table>> *out) {
  std::shared_ptr<arrow::Table> table_;
  table->ToArrowTable(table_);

  // keep arrays for each target, these arrays are used for creating the table
  std::unordered_map<int, std::shared_ptr<std::vector<std::shared_ptr<arrow::Array>>>> data_arrays;
  std::vector<int> partitions;
  for (int t = 0; t < no_of_partitions; t++) {
    partitions.push_back(t);
    data_arrays.insert(
        std::pair<int, std::shared_ptr<std::vector<std::shared_ptr<arrow::Array>>>>(
            t, std::make_shared<std::vector<std::shared_ptr<arrow::Array>>>()));
  }

  std::vector<std::shared_ptr<arrow::Array>> arrays;
  int64_t length = 0;
  for (auto col_index : hash_columns) {
    auto column = table_->column(col_index);
    std::shared_ptr<arrow::Array> array = column->chunk(0);
    arrays.push_back(array);

    if (!(length == 0 || length == column->length())) {
      return Status(cylon::IndexError,
                    "Column lengths doesnt match " + std::to_string(length));
    }
    length = column->length();
  }

  // first we partition the table
  std::vector<int64_t> outPartitions;
  std::vector<uint32_t> counts(no_of_partitions, 0);

  outPartitions.reserve(length);
  Status status = HashPartitionArrays(cylon::ToArrowPool(ctx), arrays, length,
                                      partitions, &outPartitions, counts);
  if (!status.is_ok()) {
    LOG(FATAL) << "Failed to create the hash partition";
    return status;
  }

  for (int i = 0; i < table_->num_columns(); i++) {
    std::shared_ptr<arrow::DataType> type = table_->column(i)->chunk(0)->type();
    std::shared_ptr<arrow::Array> array = table_->column(i)->chunk(0);

    std::shared_ptr<ArrowArraySplitKernel> splitKernel;
    status = CreateSplitter(type, cylon::ToArrowPool(ctx), &splitKernel);
    if (!status.is_ok()) {
      LOG(FATAL) << "Failed to create the splitter";
      return status;
    }

    // this one outputs arrays for each target as a map
    std::unordered_map<int, std::shared_ptr<arrow::Array>> splited_arrays;

    splitKernel->Split(array, outPartitions, partitions, splited_arrays, counts);

    for (const auto &x : splited_arrays) {
      std::shared_ptr<std::vector<std::shared_ptr<arrow::Array>>> cols = data_arrays[x.first];
      cols->push_back(x.second);
    }
  }
  // now insert these array to
  for (const auto &x : data_arrays) {
    std::shared_ptr<arrow::Table> final_arrow_table = arrow::Table::Make(table_->schema(), *x.second);
    std::shared_ptr<cylon::Table> kY = std::make_shared<cylon::Table>(final_arrow_table, ctx);
    out->insert(std::pair<int, std::shared_ptr<cylon::Table>>(x.first, kY));
  }
  return Status::OK();
}

Status HashPartition(std::shared_ptr<CylonContext> &ctx, const std::shared_ptr<Table> &table,
                     int hash_column, int no_of_partitions,
                     std::unordered_map<int, std::shared_ptr<cylon::Table>> *out) {
  std::shared_ptr<arrow::Table> table_;
  table->ToArrowTable(table_);

  // keep arrays for each target, these arrays are used for creating the table
  std::unordered_map<int, std::shared_ptr<std::vector<std::shared_ptr<arrow::Array>>>> data_arrays;
  std::vector<int> partitions;
  for (int t = 0; t < no_of_partitions; t++) {
    partitions.push_back(t);
    data_arrays.insert(
        std::pair<int, std::shared_ptr<std::vector<std::shared_ptr<arrow::Array>>>>(
            t, std::make_shared<std::vector<std::shared_ptr<arrow::Array>>>()));
  }

  int64_t length = 0;
  auto column = table_->column(hash_column);
  std::shared_ptr<arrow::Array> arr = column->chunk(0);
  length = column->length();

  // first we partition the table
  std::vector<int64_t> outPartitions;
  std::vector<uint32_t> counts(no_of_partitions, 0);

  outPartitions.reserve(length);
  Status status = HashPartitionArray(cylon::ToArrowPool(ctx), arr,
                                      partitions, &outPartitions, counts);
  if (!status.is_ok()) {
    LOG(FATAL) << "Failed to create the hash partition";
    return status;
  }

  for (int i = 0; i < table_->num_columns(); i++) {
    std::shared_ptr<arrow::DataType> type = table_->column(i)->chunk(0)->type();
    std::shared_ptr<arrow::Array> array = table_->column(i)->chunk(0);
    std::shared_ptr<ArrowArraySplitKernel> splitKernel;
    status = CreateSplitter(type, cylon::ToArrowPool(ctx), &splitKernel);
    if (!status.is_ok()) {
      LOG(FATAL) << "Failed to create the splitter";
      return status;
    }

    // this one outputs arrays for each target as a map
    std::unordered_map<int, std::shared_ptr<arrow::Array>> splited_arrays;

    splitKernel->Split(array, outPartitions, partitions, splited_arrays, counts);

    for (const auto &x : splited_arrays) {
      std::shared_ptr<std::vector<std::shared_ptr<arrow::Array>>> cols = data_arrays[x.first];
      cols->push_back(x.second);
    }
  }
  // now insert these array to
  for (const auto &x : data_arrays) {
    std::shared_ptr<arrow::Table> final_arrow_table = arrow::Table::Make(table_->schema(), *x.second);
    std::shared_ptr<cylon::Table> kY = std::make_shared<cylon::Table>(final_arrow_table, ctx);
    out->insert(std::pair<int, std::shared_ptr<cylon::Table>>(x.first, kY));
  }
  return Status::OK();
}
}
}*/


cylon::kernel::StreamingHashPartitionKernel::StreamingHashPartitionKernel(const std::shared_ptr<cylon::CylonContext> &ctx,
                                                                          const std::shared_ptr<arrow::Schema> &schema,
                                                                          int num_partitions,
                                                                          const std::vector<int> &hash_columns)
    : num_partitions(num_partitions), hash_columns(hash_columns), schema(schema), ctx(ctx) {
  partition_kernels.reserve(hash_columns.size());
  for (auto &&col:hash_columns) {
    partition_kernels.emplace_back(CreateHashPartitionKernel(schema->field(col)->type()));
  }

  split_kernels.reserve(schema->num_fields());
  for (auto &&field:schema->fields()) {
    split_kernels.emplace_back(CreateStreamingSplitter(field->type(),
                                                       num_partitions,
                                                       cylon::ToArrowPool(ctx)));
  }

  temp_partition_hist.resize(num_partitions, 0);
}

cylon::Status cylon::kernel::StreamingHashPartitionKernel::Process(int tag, const std::shared_ptr<Table> &table) {
  const std::shared_ptr<arrow::Table> &arrow_table = table->get_table();

  if (arrow_table->column(0)->num_chunks() > 1) {
    return Status(Code::Invalid, "chunked arrays not supported"); // todo check this!
  }

  // first resize the vectors for partitions and counts
  temp_partitions.resize(arrow_table->num_rows());
  std::fill(temp_partitions.begin(), temp_partitions.end(), 0);
  std::fill(temp_partition_hist.begin(), temp_partition_hist.end(), 0);

  // building hash without the last hash_column_idx
  for (size_t i = 0; i < hash_columns.size() - 1; i++) {
    RETURN_CYLON_STATUS_IF_FAILED(partition_kernels[i]
                                      ->UpdateHash(arrow_table->column(hash_columns[i]), temp_partitions))
  }

  // build hash from the last hash_column_idx
  RETURN_CYLON_STATUS_IF_FAILED(partition_kernels.back()->Partition(
      arrow_table->column(hash_columns.back()),
      num_partitions,
      temp_partitions,
      temp_partition_hist))

  // now insert table to split_kernels
  for (int i = 0; i < arrow_table->num_columns(); i++) {
    RETURN_CYLON_STATUS_IF_FAILED(split_kernels[i]->Split(
        arrow_table->column(i)->chunk(0), temp_partitions, temp_partition_hist))
  }

  return Status::OK();
}

cylon::Status cylon::kernel::StreamingHashPartitionKernel::Finish(std::vector<std::shared_ptr<Table>> &partitioned_tables) {
  // no longer needs the temp arrays
  temp_partitions.clear();
  temp_partition_hist.clear();

  // call streaming split kernel finalize and create the final arrays
  std::vector<arrow::ArrayVector> data_arrays(num_partitions); // size num_partitions

  std::vector<std::shared_ptr<arrow::Array>> temp_split_arrays;
  for (const auto &splitKernel:split_kernels) {
    RETURN_CYLON_STATUS_IF_FAILED(splitKernel->Finish(temp_split_arrays))
    for (size_t i = 0; i < temp_split_arrays.size(); i++) {
      // remove the array and put it in the data arrays vector
      data_arrays[i].emplace_back(std::move(temp_split_arrays[i]));
    }
    temp_split_arrays.clear();
  }

  partitioned_tables.reserve(num_partitions);
  for (const auto &arr_vec: data_arrays) {
    auto a_table = arrow::Table::Make(schema, arr_vec);
    partitioned_tables.emplace_back(std::make_shared<Table>(a_table, ctx));
  }

  return cylon::Status::OK();
}