#pragma once

#include <algorithm>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Tangram {

/**
 * Given a list of edges describing a directed acyclic graph, returns a
 * list of nodes sorted in topological order; that is, if there is a path
 * from node 'a' to node 'b' in the graph, then 'a' will always precede
 * 'b' in the sorted result.
 */
template<typename T>
std::vector<T> topologicalSort(std::vector<std::pair<T, T>> edges) {

    // Produce a sorted list of nodes using Kahn's algorithm.
    std::vector<T> sorted;

    // Loop over input edges, counting the incoming edges for each node.
    std::unordered_map<T, size_t> incoming_edges;
    for (const auto& edge : edges) {
        incoming_edges[edge.first];
        ++incoming_edges[edge.second];
    }

    // Find the set of starting nodes that have no incoming edges.
    std::vector<T> start_nodes;
    for (const auto& node : incoming_edges) {
        if (node.second == 0) {
            start_nodes.push_back(node.first);
        }
    }

    while (!start_nodes.empty()) {
        // Choose a node 'n' from the set of starting nodes and remove it.
        auto n = start_nodes.back();
        start_nodes.pop_back();

        // Add the value of 'n' to the sorted list.
        sorted.push_back(n);

        // For each node 'm' with an edge 'e' from 'n' to 'm' ...
        for (auto it = edges.begin(); it != edges.end(); ) {
            auto e = *it;
            if (e.first == n) {
                auto m = e.second;
                // ... remove edge 'e' from the graph.
                it = edges.erase(it);
                // If 'm' has no more incoming edges,
                // add it to the set of starting nodes.
                if (--incoming_edges[m] == 0) {
                    start_nodes.push_back(m);
                }
            } else {
                ++it;
            }
        }
    }

    if (!edges.empty()) {
        // Graph has cycles! This is an error.
        return {};
    }

    return sorted;

}

}
