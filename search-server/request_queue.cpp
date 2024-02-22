#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server)
    :search_server_(search_server),
    time_count_(0),
    no_results_requests_(0)
{
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    const std::vector<Document> result = search_server_.FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status,
        int rating) { return document_status == status; });
    AddRequest(result.size());
    return result;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    const std::vector<Document> result = search_server_.FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    AddRequest(result.size());
    return result;

}

int RequestQueue::GetNoResultRequests() const {
    int empty = 0;
    if (time_count_ < min_in_day_) {
        for (const auto& req : requests_) {
            if (req.size_of_result == 0) {
                ++empty;
            }
        }
    }
    else {
        for (const auto& req : requests_) {
            if (req.size_of_result == 0 && req.time > (time_count_ - min_in_day_)) {
                ++empty;
            }
        }
    }
    return empty;
}

void RequestQueue::AddRequest(int size) {
    ++time_count_;
    if (time_count_ <= min_in_day_) {
        requests_.push_back(QueryResult{ time_count_, size });
    }
    else {
        requests_.pop_front();
        requests_.push_back(QueryResult{ time_count_, size });
    }
}
