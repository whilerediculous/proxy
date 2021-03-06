/* Copyright 2019 Istio Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "extensions/stackdriver/metric/record.h"

#include "extensions/stackdriver/common/constants.h"
#include "extensions/stackdriver/metric/registry.h"
#include "google/protobuf/util/time_util.h"

using google::protobuf::util::TimeUtil;

namespace Extensions {
namespace Stackdriver {
namespace Metric {

constexpr char kCanonicalNameLabel[] = "service.istio.io/canonical-name";
constexpr char kCanonicalRevisionLabel[] =
    "service.istio.io/canonical-revision";
constexpr char kLatest[] = "latest";

void record(bool is_outbound, const ::Wasm::Common::FlatNode& local_node_info,
            const ::Wasm::Common::FlatNode& peer_node_info,
            const ::Wasm::Common::RequestInfo& request_info) {
  double latency_ms = request_info.duration /* in nanoseconds */ / 1000000.0;
  const auto& operation =
      request_info.request_protocol == ::Wasm::Common::kProtocolGRPC
          ? request_info.request_url_path
          : request_info.request_operation;

  const auto local_labels = local_node_info.labels();
  const auto peer_labels = peer_node_info.labels();

  const auto local_name_iter =
      local_labels ? local_labels->LookupByKey(kCanonicalNameLabel) : nullptr;
  const auto local_canonical_name = local_name_iter
                                        ? local_name_iter->value()
                                        : local_node_info.workload_name();

  const auto peer_name_iter =
      peer_labels ? peer_labels->LookupByKey(kCanonicalNameLabel) : nullptr;
  const auto peer_canonical_name =
      peer_name_iter ? peer_name_iter->value() : peer_node_info.workload_name();

  const auto local_rev_iter =
      local_labels ? local_labels->LookupByKey(kCanonicalRevisionLabel)
                   : nullptr;
  const auto local_canonical_rev =
      local_rev_iter ? local_rev_iter->value() : nullptr;

  const auto peer_rev_iter =
      peer_labels ? peer_labels->LookupByKey(kCanonicalRevisionLabel) : nullptr;
  const auto peer_canonical_rev =
      peer_rev_iter ? peer_rev_iter->value() : nullptr;

  if (is_outbound) {
    opencensus::stats::Record(
        {{clientRequestCountMeasure(), 1},
         {clientRequestBytesMeasure(), request_info.request_size},
         {clientResponseBytesMeasure(), request_info.response_size},
         {clientRoundtripLatenciesMeasure(), latency_ms}},
        {{meshUIDKey(), flatbuffers::GetString(local_node_info.mesh_id())},
         {requestOperationKey(), operation},
         {requestProtocolKey(), request_info.request_protocol},
         {serviceAuthenticationPolicyKey(),
          ::Wasm::Common::AuthenticationPolicyString(
              request_info.service_auth_policy)},
         {destinationServiceNameKey(), request_info.destination_service_name},
         {destinationServiceNamespaceKey(),
          flatbuffers::GetString(peer_node_info.namespace_())},
         {destinationPortKey(), std::to_string(request_info.destination_port)},
         {responseCodeKey(), std::to_string(request_info.response_code)},
         {sourcePrincipalKey(), request_info.source_principal},
         {sourceWorkloadNameKey(),
          flatbuffers::GetString(local_node_info.workload_name())},
         {sourceWorkloadNamespaceKey(),
          flatbuffers::GetString(local_node_info.namespace_())},
         {sourceOwnerKey(), flatbuffers::GetString(local_node_info.owner())},
         {destinationPrincipalKey(), request_info.destination_principal},
         {destinationWorkloadNameKey(),
          flatbuffers::GetString(peer_node_info.workload_name())},
         {destinationWorkloadNamespaceKey(),
          flatbuffers::GetString(peer_node_info.namespace_())},
         {destinationOwnerKey(),
          flatbuffers::GetString(peer_node_info.owner())},
         {destinationCanonicalServiceNameKey(),
          flatbuffers::GetString(peer_canonical_name)},
         {destinationCanonicalServiceNamespaceKey(),
          flatbuffers::GetString(peer_node_info.namespace_())},
         {destinationCanonicalRevisionKey(),
          peer_canonical_rev ? peer_canonical_rev->str() : kLatest},
         {sourceCanonicalServiceNameKey(),
          flatbuffers::GetString(local_canonical_name)},
         {sourceCanonicalServiceNamespaceKey(),
          flatbuffers::GetString(local_node_info.namespace_())},
         {sourceCanonicalRevisionKey(),
          local_canonical_rev ? local_canonical_rev->str() : kLatest}});
    return;
  }

  opencensus::stats::Record(
      {{serverRequestCountMeasure(), 1},
       {serverRequestBytesMeasure(), request_info.request_size},
       {serverResponseBytesMeasure(), request_info.response_size},
       {serverResponseLatenciesMeasure(), latency_ms}},
      {{meshUIDKey(), flatbuffers::GetString(local_node_info.mesh_id())},
       {requestOperationKey(), operation},
       {requestProtocolKey(), request_info.request_protocol},
       {serviceAuthenticationPolicyKey(),
        ::Wasm::Common::AuthenticationPolicyString(
            request_info.service_auth_policy)},
       {destinationServiceNameKey(), request_info.destination_service_name},
       {destinationServiceNamespaceKey(),
        flatbuffers::GetString(local_node_info.namespace_())},
       {destinationPortKey(), std::to_string(request_info.destination_port)},
       {responseCodeKey(), std::to_string(request_info.response_code)},
       {sourcePrincipalKey(), request_info.source_principal},
       {sourceWorkloadNameKey(),
        flatbuffers::GetString(peer_node_info.workload_name())},
       {sourceWorkloadNamespaceKey(),
        flatbuffers::GetString(peer_node_info.namespace_())},
       {sourceOwnerKey(), flatbuffers::GetString(peer_node_info.owner())},
       {destinationPrincipalKey(), request_info.destination_principal},
       {destinationWorkloadNameKey(),
        flatbuffers::GetString(local_node_info.workload_name())},
       {destinationWorkloadNamespaceKey(),
        flatbuffers::GetString(local_node_info.namespace_())},
       {destinationOwnerKey(), flatbuffers::GetString(local_node_info.owner())},
       {destinationCanonicalServiceNameKey(),
        flatbuffers::GetString(local_canonical_name)},
       {destinationCanonicalServiceNamespaceKey(),
        flatbuffers::GetString(local_node_info.namespace_())},
       {destinationCanonicalRevisionKey(),
        local_canonical_rev ? local_canonical_rev->str() : kLatest},
       {sourceCanonicalServiceNameKey(),
        flatbuffers::GetString(peer_canonical_name)},
       {sourceCanonicalServiceNamespaceKey(),
        flatbuffers::GetString(peer_node_info.namespace_())},
       {sourceCanonicalRevisionKey(),
        peer_canonical_rev ? peer_canonical_rev->str() : kLatest}});
}

}  // namespace Metric
}  // namespace Stackdriver
}  // namespace Extensions
