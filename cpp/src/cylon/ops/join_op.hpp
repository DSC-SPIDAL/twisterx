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

#ifndef CYLON_SRC_CYLON_OPS_JOIN_OP_HPP_
#define CYLON_SRC_CYLON_OPS_JOIN_OP_HPP_

#include "parallel_op.hpp"
#include "partition_op.hpp"

namespace cylon {

class JoinOpConfig {
 private:
  int32_t join_column;

 public:
  JoinOpConfig(int32_t join_column);
  int32_t GetJoinColumn() const;
};

class JoinOp : public Op {
 private:
  std::shared_ptr<JoinOpConfig> config;

 public:
  JoinOp(std::shared_ptr<CylonContext> ctx,
         std::shared_ptr<arrow::Schema> schema,
         int32_t  id,
         std::shared_ptr<ResultsCallback> callback, std::shared_ptr<JoinOpConfig> config);

  bool Execute(int tag, std::shared_ptr<Table> table) override;

  void OnParentsFinalized() override;

  bool Finalize() override;
};
}
#endif //CYLON_SRC_CYLON_OPS_JOIN_OP_HPP_