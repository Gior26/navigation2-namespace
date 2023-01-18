// Copyright (c) 2023, Samsung Research America
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <string>
#include <memory>
#include <vector>

#include "nav2_route/edge_scorer.hpp"

namespace nav2_route
{

EdgeScorer::EdgeScorer(nav2_util::LifecycleNode::SharedPtr node)
: plugin_loader_("nav2_route", "nav2_route::EdgeCostFunction")
{
  // load plugins with a default of the DistanceScorer
  const std::vector<std::string> default_plugin_id({"DistanceScorer"});
  const std::string default_plugin_type = "nav2_route::DistanceScorer";

  nav2_util::declare_parameter_if_not_declared(
    node, "edge_cost_functions", rclcpp::ParameterValue(default_plugin_id));
  auto edge_cost_function_ids = node->get_parameter("edge_cost_functions").as_string_array();

  if (edge_cost_function_ids == default_plugin_id) {
    nav2_util::declare_parameter_if_not_declared(
      node, default_plugin_id[0] + ".plugin", rclcpp::ParameterValue(default_plugin_type));
  }

  for (size_t i = 0; i != edge_cost_function_ids.size(); i++) {
    try {
      std::string type = nav2_util::get_plugin_type_param(
        node, edge_cost_function_ids[i]);
      EdgeCostFunction::Ptr scorer = plugin_loader_.createUniqueInstance(type);
      RCLCPP_INFO(
        node->get_logger(), "Created edge cost function plugin %s of type %s",
        edge_cost_function_ids[i].c_str(), type.c_str());
      scorer->configure(node, edge_cost_function_ids[i]);
      plugins_.push_back(std::move(scorer));
    } catch (const pluginlib::PluginlibException & ex) {
      RCLCPP_FATAL(
        node->get_logger(),
        "Failed to create edge cost function. Exception: %s", ex.what());
      exit(-1);
    }
  }
}

bool EdgeScorer::score(const EdgePtr edge, float & total_score)
{
  total_score = 0.0;
  float curr_score = 0.0;

  for (auto & plugin : plugins_) {
    plugin->prepare();
  }

  for (auto & plugin : plugins_) {
    curr_score = 0.0;
    if (plugin->score(edge, curr_score)) {
      total_score += curr_score;
    } else {
      return false;
    }
  }

  return true;
}

int EdgeScorer::numPlugins() const
{
  return plugins_.size();
}

}  // namespace nav2_route
