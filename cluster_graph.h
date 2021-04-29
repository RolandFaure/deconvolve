#ifndef CLUSTER_GRAPH_H
#define CLUSTER_GRAPH_H

#include "tools.h"
#include <unordered_map>
#include <unordered_set>

void cluster_graph(robin_hood::unordered_map<long int, std::unordered_set<int>> &matching_tags, std::vector<int> &clusters);
void cluster_graph_chinese_whispers(robin_hood::unordered_map<long int, std::unordered_set<int>> &matching_tags, std::vector<int> &clusters, std::string &tag);

//functions to find connected components
void find_connected_components(std::vector<std::vector<int>> &adjMatrix, std::vector<int> &clusters);
void extend_cluster_from_seed(int seed, std::vector<std::vector<int>> &adjMatrix, std::vector<int> &clusters);

int find_threshold(float proportion, std::vector<int> &strengths_of_links);
#endif
