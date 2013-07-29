#include "CategoryClassifyScorer.h"
#include "../category-classify/CategoryClassifyTable.h"
#include <iostream>

using namespace sf1r;

namespace
{
const score_t kReduceScoreForNotRule = 0.01;
}

CategoryClassifyScorer::CategoryClassifyScorer(
    const ProductScoreConfig& config,
    const CategoryClassifyTable& categoryClassifyTable,
    const CategoryScoreMap& categoryScoreMap)
    : ProductScorer(config)
    , categoryClassifyTable_(categoryClassifyTable)
    , categoryScoreMap_(categoryScoreMap)
{
}

score_t CategoryClassifyScorer::score(docid_t docId)
{
    const CategoryClassifyTable::category_rflag_t& categoryRFlag =
        categoryClassifyTable_.getCategoryNoLock(docId);

    CategoryScoreMap::const_iterator it =
        categoryScoreMap_.find(categoryRFlag.first);

    if (it == categoryScoreMap_.end())
        return 0;

    score_t result = it->second;
    if (!categoryRFlag.second)
    {
        result -= kReduceScoreForNotRule;
    }
    int int_res = static_cast<int>(result*10000);
    score_t re_result = (double(int_res))/10000.0;
    return re_result;
}