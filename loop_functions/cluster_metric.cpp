/**
 * THIS IS THE ACTUAL METRIC WE USED IN OUR PAPER
 * SEE EQUATION (4)
 */
#include <stack>

#include "cluster_metric.h"
#include "segregation_loop_function.h"

namespace argos {

size_t find_largest_component(int **A, const size_t n) {
  // use DFS to find components and count their size
  unsigned int largest_componenet_size = 0;
  bool visited[n];
  for (size_t i = 0; i < n; ++i) {
    visited[i] = false;
  }
  for (size_t i = 0; i < n; ++i) {
    auto r = i;
    if (!visited[r]) {
      unsigned int component_size = 0;
      std::stack<int> stack;
      stack.push(r);
      while (!stack.empty()) {
        auto s = stack.top();
        stack.pop();
        if (!visited[s]) {
          component_size++;
          visited[s] = true;
          for (size_t j = 0; j < n; j++) {
            if (A[s][j] == 1) {
              stack.push(j);
            }
          }
        }
      }

      if (component_size > largest_componenet_size) {
        largest_componenet_size = component_size;
      }
    }
  }

  return largest_componenet_size;
}

Real cluster_metric(const GroupPosMap groups) {
  Real cost = 0;
  for (auto &kv : groups) {
    const auto group_id = kv.first;
    const auto poses = kv.second;

    int **A = static_cast<int **>(malloc(poses.size() * sizeof(int *)));
    for (size_t i = 0; i < poses.size(); ++i) {
      A[i] = static_cast<int *>(calloc(poses.size(), sizeof(int)));
    }

    for (size_t i = 0; i < poses.size(); ++i) {
      A[i][i] = 0;
      for (size_t j = i + 1; j < poses.size(); ++j) {
        auto i_position = poses[i];
        auto j_position = poses[j];
        const auto distance = (i_position - j_position).Length();
        // we some distance threshold for what is a "cluster"
        constexpr double ROBOT_RADIUS = 0.085036758f;
        const auto a = (distance < 2 * ROBOT_RADIUS + .05) ? 1 : 0;
        A[i][j] = a;
        A[j][i] = a;
      }
    }

    const auto largest_component_size = find_largest_component(A, poses.size());
    for (size_t i = 0; i < poses.size(); ++i) {
      free(A[i]);
    }
    free(A);

    auto const cost_t = -static_cast<Real>(largest_component_size) / poses.size();
    cost += cost_t;
  }

  return cost / groups.size();
}

}
