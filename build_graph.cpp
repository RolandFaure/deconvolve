#include "build_graph.h"
#include "cluster_graph.h"

#include <random> //for the iota function
#include <map>

using namespace std::chrono;

using std::cout;
using std::endl;
using std::vector;
using std::list;
using std::min;
using robin_hood::unordered_map;
using robin_hood::unordered_flat_map;
using std::map;
using std::string;
using std::array;
using std::unordered_set;
using std::set;
//using namespace lemon;

//the function takes as an input the list of all reads having the same tag
void build_graph(short minCommonKmers, string tag, long int tagCloud, const vector <vector<long long int>> &readClouds, const std::vector <Read> &reads, const vector<set<long int>> &kmers, vector<int> &clusters){
	
	auto t0 = high_resolution_clock::now();

    int adjMatrixSize = clusters.size();
    vector<int> zeros (adjMatrixSize, 0);
    vector<vector<int>> adjMatrix (adjMatrixSize, zeros);
		
    build_adj_matrix(minCommonKmers, tagCloud, readClouds, reads, kmers, adjMatrix);

    auto t1 = high_resolution_clock::now();

    cluster_graph_chinese_whispers(adjMatrix, clusters, tag);

    auto t2 = high_resolution_clock::now();

    fast_clustering(tagCloud, readClouds, reads, kmers, clusters);

    auto t3 = high_resolution_clock::now();

    cout << "Building adjacency matrix : " << duration_cast<microseconds>(t1-t0).count()/1000 << "ms, clustering the matrix : " << duration_cast<microseconds>(t2-t1).count()/1000 << "ms, fast clustering : " << duration_cast<microseconds>(t3-t2).count()/1000 << "ms" << endl;
	
    if (adjMatrix.size()>500){
        string f = "/home/zaltabar/Documents/Ecole/X/4A/stage_M2/code/evalGraphs/cluster_"+tag+"_adj.csv";
        string f2 = "/home/zaltabar/Documents/Ecole/X/4A/stage_M2/code/evalGraphs/cluster_"+tag+"_nodes.csv";
        //cout << "exporting..."  << adjMatrix.size()  << " "<< clusters.size()<< endl;
        export_as_CSV(adjMatrix, f, f2, clusters);
//        f = "/home/zaltabar/Documents/Ecole/X/4A/stage_M2/code/evalGraphs/cluster_"+tag+"_matching-tag.csv";
//        export_as_CSV(matching_tags, f);
    }
}

void build_adj_matrix(short minCommonKmers, long int tagCloud, const vector <vector<long long int>> &readClouds, const std::vector <Read> &reads, const vector<set<long int>> &kmers, vector<vector<int>> &adjMatrix){

    auto t0 = high_resolution_clock::now();

    double d = 0;
    double d2 = 0;
    unordered_map<long int, unordered_set<int>> matching_tags; //maps to each tagID the index of the reads of the cloud aligning with this tag

    //int r = 0;
    for(int r = 0, sizer = readClouds[tagCloud].size(); r<sizer ; r++){


        unordered_map<long int, int> alreadySeen; //a map to keep track of how many times the read has already been attached to that tag: you need to have at least minCommonKmers common minimizer to attach a read to a tags
        long long int name = readClouds[tagCloud][r];

        for (long int m : reads[name].minis){

            if (kmers[m].size() > 2){ //if the size is 1, it will be the tagCloud

                int c = 0;
                int cmax = 50;

//                cout << "in3, " << kmers[m].size() << " " << m << " " << kmers.size() << endl;
//                for (long int t : kmers[m]){cout << t << " ";}
//                cout << endl << "inm3" << endl;

                for (long int tag : kmers[m]){


                    if (c < cmax){

                        auto tt0 = high_resolution_clock::now();
                        c++;
                        alreadySeen[tag] += 1;

                        if (alreadySeen[tag] == minCommonKmers){ // it would be equivalent to put >= here, but a bit slower
                            matching_tags[tag].emplace(r);
                        }
                        auto tt1 = high_resolution_clock::now();
                        d += duration_cast<nanoseconds>(tt1-tt0).count();
                    }
                }
             }

        }

    }
    matching_tags[tagCloud] = {}; //this line to avoid self-loops

    auto t1 = high_resolution_clock::now();

    //now build the interaction matrix between reads
    for (robin_hood::pair<const long int, unordered_set<int>> matchs : matching_tags){

//		cout << "\nLet's look at what matched with tag " << matchs.first << endl;
        int i = 0;
        for (int a  : matchs.second) {

            int j = 0;
            for (int b : matchs.second){

                if (j>i){
                    adjMatrix[a][b] += 1;
                    adjMatrix[b][a] += 1;
                }
                j++;
            }
            i++;
        }
    }
    auto t2 = high_resolution_clock::now();

    //cout << "While building adjMat, took me " << duration_cast<microseconds>(t1-t0).count() << "us to create matching tags, among it " << int(d/1000) << "us handling maps (against potentially "<< int(d2/1000) <<"us) " << duration_cast<microseconds>(t2-t1).count() << "us to build the adjMat " << endl;
}

void fast_clustering(long int tagCloud, const std::vector <std::vector<long long int>> &readClouds, const std::vector <Read> &reads, const std::vector<std::set<long int>> &kmers, vector<int> &clusters){

    double time = 0;
    int limit = 5; //how many tags two reads can share without considering they are linked
    int overlapLimit = 50; //limit of how many tags we look at for each kmer
    vector<int> clusterReps;

    unordered_map <long int, int> alreadySeenTags; //mapping all the already seen tag to the reps where they were seen

    for(int r = 0, sizer = readClouds[tagCloud].size(); r<sizer ; r++){

        short known = 0; //bool keeping trace of with how many already seen barcodes the new read overlaps

        long long int name = readClouds[tagCloud][r];

        for (long int m : reads[name].minis){
            if (known < limit){

                known --; //that is because tagCloud will always be there as a common tag
                for (long int tag : kmers[m]){

                    if (known < limit){
                        if (alreadySeenTags.find(tag) != alreadySeenTags.end()){
                            known++;
                        }
                    }
                    else{
                        break;
                    }
                }
            }
            else{
                break;
            }
        }

        if (known < limit){ //then add all the new tags to alreadySeenTags

            clusterReps.push_back(r);
            int clustIdx = clusterReps.size()-1;
            for (long int m : reads[name].minis){
                for (long int tag : kmers[m]){
                    alreadySeenTags[tag] = clustIdx;
                }
            }
        }
    }
//    cout << "Fast clustering, the reps are : ";
//    for (int i : clusterReps){
//        cout << i << " ";
//    }
//    cout << endl;


    //now that we have all the representants of the clusters, map each read to the best cluster

    short newTagsToAdd = 3; //because rep may not always be 100% representative, allow this number of new tags to be indexed at each read to enrich the reference

    //go through all the reads by going through the vector order (we do that instead of a for loop in range(0,clusters.size()) because we'd like to re-cluster the reads we are the less sure about :
    vector<int> order (clusters.size());
    std::iota(order.begin(), order.end(), 0); //fill this vector with increasing int starting from 0

    std::map <std::pair<long int, long int>, int> fusionProposal; //a map of pairs to see if two clusters should be merged
    std::set<std::pair<long int, long int>> fusionDecided; //a map of pairs indicating which clusters should be merged

    for(int n = 0 ; n < order.size() ; n++){

        int r = order[n];

        set <long int> newTags;

        vector<int> clusterScores (clusterReps.size());

        long long int name = readClouds[tagCloud][r];

         for (long int m : reads[name].minis){

             int index = 0;
             for (auto tag : kmers[m]){

                 if (index < overlapLimit){
                     if (alreadySeenTags.find(tag) != alreadySeenTags.end()){
                         clusterScores[alreadySeenTags[tag]] += 1;
                     }
                     else if (newTags.size() < newTagsToAdd){
                         newTags.emplace(tag);
                     }
                 }
                 index++;
             }

         }
;
         //now find the cluster the read seems closest to
         int max = clusterScores[0];
         int idxmax = 0;
         int max2 = -1; //second max
         int idxMax2 = -1;
         for (int i = 1 ; i < clusterScores.size() ; i++){
             if (clusterScores[i]>max){
                 max2 = max;
                 idxMax2 = idxmax;
                 max = clusterScores[i];
                 idxmax = i;
             }
             else if (clusterScores[i] > max2){
                 max2 = clusterScores[i];
                 idxMax2 = i;
             }
         }



         if (clusterScores.size()>1 && max<max2*2 && n<2*clusters.size()){ //if the clustering is not fully convincing
             order.push_back(r);
             fusionProposal[std::make_pair(min(idxmax, idxMax2), std::max(idxmax, idxMax2))] += 1;

             //look if there is enough evidence that idxmax and idxmax2 should in fact be merged in one cluster: if not, then r remains mysterious and you may want to re-inspect it
             int fusionScore = fusionProposal[std::make_pair(min(idxmax, idxMax2), std::max(idxmax, idxMax2))];

             //first check if the fusion has already been decided anyway
             if (fusionDecided.find(std::make_pair(min(idxmax, idxMax2), std::max(idxmax, idxMax2))) == fusionDecided.end()) {

                 //then check if the fusion should be decided
                 if (fusionScore > 20 && fusionScore > 2*fusionProposal[std::make_pair(min(idxmax, idxmax), std::max(idxmax, idxmax))]+fusionProposal[std::make_pair(min(idxMax2, idxMax2), std::max(idxMax2, idxMax2))] ){

                     //then merge :
                     fusionDecided.emplace(std::make_pair(min(idxmax, idxMax2), std::max(idxmax, idxMax2)));
                 }
                 else{ //it is then still unclear what cluster r is from
                    order.push_back(r);
                 }
             }

         }
         else{
             clusters[r] = idxmax;
             fusionProposal[std::make_pair(idxmax,idxmax)] += 1;
             //enrich the reference with new tags
             for (long int nt : newTags){
                 alreadySeenTags[nt] = idxmax;
            }

         }

    }

//    cout << "I would like to merge : ";
//    for (auto i : fusionProposal) cout << i.first.first << "," << i.first.second << "," << i.second << " ; ";
//    cout << endl;
//    cout << "I merge : ";
//    for (auto i : fusionDecided) cout << i.first << "," << i.second << " ; ";
//    cout << endl;

    //merge all clusters that should be merged:

    for (auto fusion : fusionDecided){

        for (int c = 0 ; c < clusters.size() ; c++){

            if (clusters[c] == fusion.second){
                clusters[c] = fusion.first;
            }
        }
    }

}
