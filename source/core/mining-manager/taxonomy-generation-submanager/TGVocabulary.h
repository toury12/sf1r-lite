///
/// @file   TGVocabulary.h
/// @brief  Controlled vocabulary used for TG.
/// @author Jia Guo
/// @date   2010-02-03
/// @date   2010-02-03
///
#ifndef TGVOCABULARY_H_
#define TGVOCABULARY_H_
#include <string>
#include <mining-manager/MiningManagerDef.h>
#include <ctime>
#include <boost/filesystem.hpp>
#include <boost/tuple/tuple.hpp>
#include <algorithm>
#include <boost/bind.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/thread.hpp> 
#include <fstream>
#include <cache/IzeneCache.h>
#include <util/izene_serialization.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
namespace sf1r
{
    
    
class TGVocabulary
{
    public:
        TGVocabulary();
        ~TGVocabulary();
        
        void open();
        void flush();
        void close();
        
        void addSiblingLabelPair(uint32_t leftLabelId, uint32_t rightLabelId, uint32_t count = 1);
        void addFathershipLabelPair(uint32_t fatherLabelId, uint32_t childLabelId, uint32_t count = 1);

        void getSiblingRelativeLables(uint32_t labelId, std::vector<std::pair<uint32_t, uint32_t> >& result);
        void getChildRelativeLables(uint32_t labelId, std::vector<std::pair<uint32_t, uint32_t> >& result);
        
    private:
        LabelRepTable labelRepTable_;
        izenelib::am::tc_hash<uint32_t, std::vector<uint32_t> > semanticChildLabelList_;
};

    
}
#endif