#include "document.h"

Document::Document() {}

Document::Document(int id, double relevance, int rating)
        : id(id), relevance(relevance), rating(rating) 
{
}

std::ostream& operator<<(std::ostream& output, Document document) {
    output << "{ document_id = " << document.id << ", relevance = " << document.relevance << ", rating = " << document.rating << " }";
    return output;
}