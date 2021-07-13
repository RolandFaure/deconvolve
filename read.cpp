#include "read.h"

using std::vector;
using std::cout;
using std::endl;

Read::Read(int num_threads)
{
    for (int t = 0 ; t < num_threads ; t++){
        minis.push_back({});
    }
    for (int t = 0 ; t < num_threads ; t++){
        minis_seq.push_back({});
    }
    for (int t = 0 ; t < num_threads ; t++){
        minis_seq_rev.push_back({});
    }
}

void Read::new_mini(long idx, int thread_id){

    //lock the mutex : only one thread can appen to minis at the same time
    //mutex.lock();

    minis[thread_id].push_back(idx);

    //mutex.unlock();
}

std::vector<std::vector<long int>>& Read::get_minis(){
    return minis;
}

void Read::get_minis_seq(int k, std::vector<Sequence> &res, int thread_id){

    for (int pos : minis_seq[thread_id]){
        res.push_back(sequence.subseq(pos, k));
    }
    minis_seq[thread_id] = {}; //now that they are returned as sequences, you don't need to keep them at indices

    Sequence rev = sequence.reverse_complement();
    for (int pos : minis_seq_rev[thread_id]){
        res.push_back(rev.subseq(pos, k));
    }
    minis_seq_rev[thread_id] = {};
}

std::vector<std::vector<int>> Read::get_pos_minis(){
    return minis_seq;
}

void Read::reset_minis_seq_thread (int thread){
    minis_seq[thread] = {};
}

void Read::compute_minimisers(int k, int h, int w){

    sequence.minimisers(h,k,w, minis_seq);
    sequence.reverse_complement().minimisers(h,k,w,minis_seq_rev); //append to minis_seq also the minimisers of the reverse complement
}
