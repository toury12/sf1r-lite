#include "MiningSearchService.h"
#include <mining-manager/faceted-submanager/ontology_manager.h>
namespace sf1r
{

MiningSearchService::MiningSearchService()
{
}

MiningSearchService::~MiningSearchService()
{
}


bool MiningSearchService::getSearchResult(
    KeywordSearchResult& resultItem
)
{
    return miningManager_->getMiningResult(resultItem);
}

bool MiningSearchService::getSimilarDocIdList(
    uint32_t documentId, 
    uint32_t maxNum, 
    std::vector<std::pair<uint32_t, float> >& result
)
{
    return miningManager_->getSimilarDocIdList(documentId, maxNum, result);
}

bool MiningSearchService::getDuplicateDocIdList(
    uint32_t docId, 
    std::vector<uint32_t>& docIdList
)
{
    return miningManager_->getDuplicateDocIdList(docId,docIdList);
}

bool MiningSearchService::getSimilarImageDocIdList(
    const std::string& targetImageURI,
    SimilarImageDocIdList& imageDocIdList
)
{
    return true;
}

bool MiningSearchService::getReminderQuery(
    std::vector<izenelib::util::UString>& popularQueries, 
    std::vector<izenelib::util::UString>& realtimeQueries
)
{
    return true;
//     return miningManager_->getReminderQuery(popularQueries, realtimeQueries);
}

bool MiningSearchService::getSimilarLabelStringList(
    uint32_t label_id, 
    std::vector<izenelib::util::UString>& label_list 
)
{
    return miningManager_->getSimilarLabelStringList(label_id, label_list);
}

bool MiningSearchService::getDocLabelList(
    uint32_t docid, 
    std::vector<std::pair<uint32_t, izenelib::util::UString> >& label_list 
)
{
    return miningManager_->getLabelListByDocId(docid, label_list);
}

bool MiningSearchService::getLabelListWithSimByDocId(
    uint32_t docid,  
    std::vector<std::pair<izenelib::util::UString, std::vector<izenelib::util::UString> > >& label_list
)
{
    return miningManager_->getLabelListWithSimByDocId(docid, label_list);
}

bool MiningSearchService::getUniqueDocIdList(
    const std::vector<uint32_t>& docIdList, 
    std::vector<uint32_t>& cleanDocs
)
{
    return miningManager_->getUniqueDocIdList(docIdList, cleanDocs);
}

bool MiningSearchService::SetOntology(const std::string& xml)
{
    boost::shared_ptr<faceted::OntologyManager> faceted = miningManager_->GetFaceted();
    if(!faceted)
    {
        return false;
    }
    faceted->SetXML(xml);
    return true;
}

bool MiningSearchService::GetOntology(std::string& xml)
{
    boost::shared_ptr<faceted::OntologyManager> faceted = miningManager_->GetFaceted();
    if(!faceted)
    {
        return false;
    }
    faceted->GetXML(xml);
    return true;
}

bool MiningSearchService::GetStaticOntologyRepresentation(faceted::OntologyRep& rep)
{
    boost::shared_ptr<faceted::OntologyManager> faceted = miningManager_->GetFaceted();
    if(!faceted)
    {
        return false;
    }
    faceted::OntologySearcher* searcher = faceted->GetSearcher();
    searcher->GetStaticRepresentation(rep);
    return true;
}

bool MiningSearchService::OntologyStaticClick(uint32_t cid, std::list<uint32_t>& docid_list)
{
    boost::shared_ptr<faceted::OntologyManager> faceted = miningManager_->GetFaceted();
    if(!faceted)
    {
        return false;
    }
    faceted::OntologySearcher* searcher = faceted->GetSearcher();
    searcher->StaticClick(cid, docid_list);
    return true;
}

bool MiningSearchService::GetOntologyRepresentation(
    const std::vector<uint32_t>& search_result, 
    faceted::OntologyRep& rep
)
{
    boost::shared_ptr<faceted::OntologyManager> faceted = miningManager_->GetFaceted();
    if(!faceted)
    {
        return false;
    }
    faceted::OntologySearcher* searcher = faceted->GetSearcher();
    searcher->GetRepresentation(search_result, rep);
    return true;
}

bool MiningSearchService::OntologyClick(
    const std::vector<uint32_t>& search_result, 
    uint32_t cid, 
    std::list<uint32_t>& docid_list
)
{
    boost::shared_ptr<faceted::OntologyManager> faceted = miningManager_->GetFaceted();
    if(!faceted)
    {
        return false;
    }
    faceted::OntologySearcher* searcher = faceted->GetSearcher();
    searcher->Click(search_result, cid, docid_list);
    return true;
}

bool MiningSearchService::DefineDocCategory(
    const std::vector<faceted::ManmadeDocCategoryItem>& items
)
{
    boost::shared_ptr<faceted::OntologyManager> faceted = miningManager_->GetFaceted();
    if(!faceted)
    {
        return false;
    }
    faceted->DefineDocCategory(items);
    return true;
}

bool MiningSearchService::clickGroupLabel(
    const std::string& query,
    const std::string& propName,
    const std::string& propValue
)
{
    return miningManager_->clickGroupLabel(query, propName, propValue);
}

bool MiningSearchService::getFreqGroupLabel(
    const std::string& query,
    const std::string& propName,
    int limit,
    std::vector<std::string>& propValueVec,
    std::vector<int>& freqVec
)
{
    return miningManager_->getFreqGroupLabel(query, propName, limit, propValueVec, freqVec);
}

bool MiningSearchService::GetTdtInTimeRange(const izenelib::util::UString& start, const izenelib::util::UString& end, std::vector<izenelib::util::UString>& topic_list)
{
    return miningManager_->GetTdtInTimeRange(start, end, topic_list);
}

bool MiningSearchService::GetTdtTopicInfo(const izenelib::util::UString& text, idmlib::tdt::TopicInfoType& topic_info)
{
    return miningManager_->GetTdtTopicInfo(text, topic_info);
}

void MiningSearchService::InjectQueryRecommend(const izenelib::util::UString& query, const izenelib::util::UString& result)
{
    miningManager_->InjectQueryRecommend(query, result);
}
    
void MiningSearchService::FinishQueryRecommendInject()
{
    miningManager_->FinishQueryRecommendInject();
}

}

