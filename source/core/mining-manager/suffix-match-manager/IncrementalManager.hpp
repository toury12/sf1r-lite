#include <iostream>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <set>
#include <algorithm>
#include <stdio.h>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <ir/id_manager/IDManager.h>
#include <configuration-manager/PropertyConfig.h>
#include <util/ClockTimer.h>
#include <boost/filesystem.hpp>
namespace cma
{
class Analyzer;
class Knowledge;
}
/**
@brief The search result from IncrementIndex just need two parts: vector<uint32_t> ResultList and vectort<similarity> ResultList;
*/

namespace sf1r
{
#define MAX_INCREMENT_DOC 500000
#define MAX_TMP_DOC 5000000
#define MAX_POS_LEN 24
#define MAX_HIT_NUMBER 3
#define MAX_SUB_LEN 15

class DocumentManager;
class LAManager;
using izenelib::ir::idmanager::IDManager;
using namespace boost::filesystem;

/**
@brife use this 24 bit data to mark if is this position is hit or not;
postion 0-23;
*/
struct BitMap
{
    unsigned int data_;//init is zero

    unsigned int offset_;

    bool getBitMap(unsigned short i) const
    {
        if (i > MAX_POS_LEN)
        {
            return false;
        }
        unsigned int data = 0;
        data = data_>>i;
        unsigned int j = 1;
        return (data&j) == 1;
    }
    bool setBitMap(unsigned short i)
    {
        if (i > MAX_POS_LEN)
        {
            return false;
        }
        unsigned int data = 1;
        data<<=i;
        data_ |= data;
        return true;
    }

    void reset()
    {
        data_ = 0;
    }
};
/**
@brief IndexItem for forwardIndex;
*/
struct IndexItem
{
    uint32_t Docid_;

    uint32_t Termid_;

    unsigned short Pos_;

    void setIndexItem_(uint32_t docid, uint32_t termid, unsigned short pos)
    {
        Docid_ = docid;
        Termid_ = termid;
        Pos_ = pos;
    }

    IndexItem()
    {
        Docid_ = 0;
        Termid_ = 0;
        Pos_ = 0;
    }

    IndexItem(const IndexItem& indexItem)
    {
        *this = indexItem;
    }
};

struct pairLess
{
    bool operator() (const pair<unsigned int, unsigned short>&s1, const pair<unsigned int, unsigned short>&s2) const
    {
        return s1.first < s2.first;
    }
};

/**
@brief ForwardIndex is for restore query
*/
class ForwardIndex
{
public:
    ForwardIndex(std::string index_path, unsigned int Max_Doc_Num)
    {
        index_path_ = index_path + ".fd.idx";
        Max_Doc_Num_ = Max_Doc_Num;
        IndexCount_ = 0;
        IndexItems_.clear();
        BitMapMatrix_ = new BitMap[Max_Doc_Num_];
        start_docid_ = 0;
        max_docid_ = 0;
    }

    ~ForwardIndex()
    {
        if (BitMapMatrix_ != NULL)
        {
            delete [] BitMapMatrix_;
        }
        BitMapMatrix_ = NULL;
    }

    bool checkLocalIndex_()
    {
        if(boost::filesystem::exists(index_path_))
        {
            if (load_())
            {
                return true;
            }
            else
            {
                boost::filesystem::remove(index_path_);
                return false;
            }
        }
        else
            return false;
    }

    uint32_t getIndexCount_()
    {
        return IndexCount_;
    }

    void addDocForwardIndex_(uint32_t docId, std::vector<uint32_t>& termidList, std::vector<unsigned short>& posList)
    {
        if (start_docid_ == 0)
        {
            start_docid_ = docId;
        }

        BitMapMatrix_[docId - start_docid_].offset_ = IndexCount_;
        unsigned int count = termidList.size();
        for (unsigned int i = 0; i < count; ++i)
        {
            addOneIndexIterms_(docId, termidList[i], posList[i]);
            IndexCount_++;
        }
        max_docid_ = docId;
    }

    void addOneIndexIterms_(uint32_t docId, uint32_t termid, unsigned short pos)
    {
        IndexItem indexItem;
        indexItem.setIndexItem_(docId, termid, pos);
        IndexItems_.push_back(indexItem);
    }

    uint32_t getSorce(std::vector<uint32_t> &v, uint32_t & docid)
    {
        uint32_t start = BitMapMatrix_[docid - start_docid_].offset_;
        uint32_t end = BitMapMatrix_[docid - start_docid_ + 1].offset_;
        std::vector<uint32_t> doclist;
        for (uint32_t i = start; i < end; ++i)
        {
            doclist.push_back(IndexItems_[i].Termid_);
        }
        return includeNum(v, doclist);
    }

    uint32_t includeNum(const vector<uint32_t>& vec1,const vector<uint32_t>& vec2)
    {
        uint32_t k = 0;
        uint32_t j = 0;
        for (uint32_t i = 0; i < vec2.size(); ++i)
        {
            if (j < vec1.size())
            {
                while (vec1[j] == vec2[i])
                {
                    if (j == vec1.size() - 1)
                    {
                        k++;
                        j = 0;
                        break;
                    }
                    j++;
                    i++;
                }
            }
            else
                continue;
        }
        return k;
    }


    void setPosition_(std::vector<pair<uint32_t, unsigned short> >*& v)
    {
        std::vector<pair<uint32_t, unsigned short> >::const_iterator it = (*v).begin();
        for (; it != (*v).end(); ++it)
        {
            BitMapMatrix_[(*it).first - start_docid_].setBitMap((*it).second);//ADD OFFSET.....
        }
    }

    bool getTermIDByPos_(const uint32_t& docid, const std::vector<unsigned short>& pos, std::vector<uint32_t>& termidList)
    {
        std::set<uint32_t> termidSet;
        std::vector<IndexItem>::const_iterator it = IndexItems_.begin();
        std::vector<IndexItem>::const_iterator its = IndexItems_.begin();
        for (; it != IndexItems_.end(); ++its,++it)
        {
            if ((*it).Docid_ == docid)
                break;
        }
        for (std::vector<unsigned short>::const_iterator i = pos.begin(); i != pos.end(); ++i)
        {
            uint32_t k = 0;
            it = its;
            while(k < MAX_POS_LEN)
            {
                if ((*it).Docid_ == docid && (*it).Pos_ == *i)
                {
                    termidSet.insert((*it).Termid_);
                    break;
                }
                k++;
                it++;
            }
        }
        for (std::set<uint32_t>::iterator i = termidSet.begin(); i != termidSet.end(); ++i)
        {
            termidList.push_back(*i);
        }
        return true;
    }

    void resetBitMapMatrix()
    {
        for (unsigned int i = 0; i < (max_docid_ - start_docid_) ; ++i)//for (int i = 0; i < (max_docid_ - start_docid_); i++ )....
        {
            BitMapMatrix_[i].reset();
        }
    }

    void getLongestSubString_(std::vector<uint32_t>& termidList, const std::vector<std::vector<pair<uint32_t, unsigned short> >* >& resultList, uint32_t& hitdoc)
    {
        std::vector<pair<uint32_t, unsigned short> >::const_iterator it;
        BitMap b;
        b.reset();
        vector<unsigned short> temp;
        vector<unsigned short> longest;
        uint32_t resultDocid = 0;
        for (std::vector<std::vector<pair<uint32_t, unsigned short> >* >::const_iterator iter = resultList.begin(); iter != resultList.end(); ++iter)
        {
            if (longest.size() > MAX_SUB_LEN)
            {
                break;
            }
            for (it = (**iter).begin(); it != (**iter).end(); ++it)
            {
                temp.clear();
                bool isNext = true;
                for (unsigned short i = 0; i < MAX_POS_LEN; ++i)
                {
                    if(BitMapMatrix_[(*it).first - start_docid_].getBitMap(i))
                    {
                        temp.push_back(i);
                        isNext = true;
                    }
                    else
                    {
                        if(isNext == false)
                        {
                            if(temp.size() > longest.size())
                            {
                                longest = temp;
                                resultDocid = (*it).first;
                            }
                            temp.clear();
                        }
                        else
                        {
                            isNext = false;
                        }
                    }
                }
                if(temp.size() > longest.size())
                {
                    longest = temp;
                    resultDocid = (*it).first;
                    temp.clear();
                }
                if (longest.size() > MAX_SUB_LEN)
                {
                    break;
                }
            }
        }
        std::vector<unsigned short> hitPos;
        LOG(INFO)<<"[INFO] Resulst Docid:"<<resultDocid<<" And hit pos:"<<endl;
        hitdoc = resultDocid;
        for (unsigned int i = 0; (i < longest.size() && i < MAX_SUB_LEN); ++i)
        {
            cout<<longest[i]<<" ";
            hitPos.push_back(longest[i]);
        }
        cout<<endl;
        /**
        If longest.size() is less than 3, means this is almost no doc similar with this doc;
        In order to prevent get those not releated files;
        */
        if (longest.size() < 3)
        {
            return;
        }
        getTermIDByPos_(resultDocid, hitPos, termidList);
    }

    void print()
    {
        cout<<"IndexCount_:"<<IndexCount_<<endl;
    }

    bool save_(string path = "")
    {
        string index_path = index_path_ + path;
        FILE *file;
        if ((file = fopen(index_path.c_str(), "wb")) == NULL)
        {
            LOG(INFO) <<"Cannot open output file"<<endl;
            return false;
        }

        unsigned int totalSize = IndexCount_ * sizeof(IndexItem);
        fwrite(&totalSize, sizeof(totalSize), 1, file);

        fwrite(&start_docid_, sizeof(unsigned int), 1, file);
        fwrite(&max_docid_, sizeof(unsigned int), 1, file);
        for (uint32_t i = start_docid_; i <= max_docid_; ++i)
        {
            fwrite(&(BitMapMatrix_[i - start_docid_].offset_), sizeof(unsigned int), 1, file);
        }
        std::vector<IndexItem>::const_iterator it = IndexItems_.begin();
        for (; it != IndexItems_.end(); ++it)
        {
            fwrite(&(*it), sizeof(IndexItem), 1, file);
        }
        fclose(file);
        return true;
    }

    bool load_(string path = "")
    {
        string index_path = index_path_ + path;
        FILE *file;
        if ((file = fopen(index_path.c_str(), "rb")) == NULL)
        {
            LOG(INFO) <<"Cannot open output file"<<endl;
            return false;
        }
        unsigned int totalSize;
        unsigned int readSize = 0;

        readSize = fread(&totalSize, sizeof(totalSize), 1, file);
        if (readSize != 1)
        {
            LOG(INFO) <<"Fread Wrong!!"<<endl;
            return false;
        }

        readSize = fread(&start_docid_, sizeof(unsigned int), 1, file);
        readSize = fread(&max_docid_, sizeof(unsigned int), 1, file);
        for (uint32_t i = start_docid_; i <= max_docid_; ++i)
        {
            readSize = fread(&(BitMapMatrix_[i - start_docid_].offset_), sizeof(unsigned int), 1, file);
        }
        IndexCount_ = totalSize/(sizeof(IndexItem));
        for (unsigned int i = 0; i < IndexCount_; ++i)
        {
            IndexItem indexItem;
            readSize = fread(&indexItem, sizeof(IndexItem), 1, file);

            IndexItems_.push_back(indexItem);
        }
        fclose(file);
        return true;
    }

    void addIndexItem_(const IndexItem& indexItem)
    {
        IndexItems_.push_back(indexItem);
        if (start_docid_ == 0)
        {
            start_docid_ = indexItem.Docid_;
        }
        if (indexItem.Docid_ > max_docid_)
        {
            max_docid_ = indexItem.Docid_;
        }
        IndexCount_++;
    }

    std::vector<IndexItem>& getIndexItem_()
    {
        return IndexItems_;
    }

    string getPath()
    {
        return index_path_;
    }

private:
    std::string index_path_;

    std::vector<IndexItem> IndexItems_;

    unsigned int IndexCount_;

    BitMap* BitMapMatrix_;

    unsigned int Max_Doc_Num_;

    unsigned int start_docid_;

    unsigned int max_docid_;
};

class IncrementIndex
{
public:
    IncrementIndex(std::string file_path,
                   unsigned int Max_Doc_Num,
                   cma::Analyzer* &analyzer)
        : analyzer_(analyzer)
    {
        Increment_index_path_ = file_path + ".inc.idx";
        Max_Doc_Num_ = Max_Doc_Num;
        termidNum_ = 0;
        allIndexDocNum_ = 0;
    }

    ~IncrementIndex()
    {

    }

    bool checkLocalIndex_()
    {
        if(boost::filesystem::exists(Increment_index_path_))
        {
            if (load_())
            {
                return true;
            }
            else
            {
                boost::filesystem::remove(Increment_index_path_);
                return false;
            }
        }
        else
            return false;
    }

    void addIndex_(uint32_t docid, std::vector<uint32_t> termidList, std::vector<unsigned short>& posList)
    {
        unsigned int count = termidList.size();
        allIndexDocNum_ += count;
        for (unsigned int j = 0; j < count; ++j)
        {
            pair<uint32_t, uint32_t> temp(termidList[j], 0);
            std::set<std::pair<uint32_t, uint32_t>, pairLess >::iterator i = termidList_.find(temp);;
            if (i != termidList_.end())
            {
                uint32_t offset = (*i).second;
                pair<uint32_t, unsigned short> term(docid, posList[j]);
                docidLists_[offset].push_back(term);//xxx
            }
            else
            {
                std::vector<pair<uint32_t, unsigned short> > tmpDocList;
                pair<uint32_t, unsigned short> term(docid, posList[j]);
                tmpDocList.push_back(term);
                pair<uint32_t, uint32_t> termid(termidList[j], docidLists_.size());
                termidList_.insert(termid);
                docidLists_.push_back(tmpDocList);
                termidNum_++;
            }
        }
    }

    void getResultAND_(const std::vector<uint32_t>& termidList, std::vector<uint32_t>& resultList, std::vector<double>& ResultListSimilarity)
    {
        std::vector<std::vector<pair<uint32_t, unsigned short> >* > docidLists;
        std::vector<uint32_t> sizeLists;
        uint32_t size;
        std::vector<pair<uint32_t, unsigned short> >* vresult = NULL;
        for (std::vector<uint32_t>::const_iterator i = termidList.begin(); i != termidList.end(); ++i)
        {
            if(!getTermIdResult_(*i, vresult, size))
                return;
            docidLists.push_back(vresult);
            sizeLists.push_back(size);
        }

        mergeAnd_(docidLists, resultList, ResultListSimilarity);
    }

    bool getTermIdResult_(const uint32_t& termid, std::vector<pair<uint32_t, unsigned short> >*& v, uint32_t& size)
    {
        pair<uint32_t, uint32_t> temp(termid, 0);

        std::set<std::pair<uint32_t, uint32_t> >::iterator i = termidList_.find(temp);
        if (i != termidList_.end())
        {
            v = &(docidLists_[(*i).second]);
            size = docidLists_[(*i).second].size();
            return true;
        }
        else
        {
            return false;
        }
    }

    void getTermIdPos_(const uint32_t& termid, std::vector<pair<uint32_t, unsigned short> >*& v)
    {
        pair<uint32_t, uint32_t> temp(termid, 0);
        std::set<std::pair<uint32_t, uint32_t> >::iterator i = termidList_.find(temp);
        if (i != termidList_.end())
        {
            v = &(docidLists_[(*i).second]);
        }
    }

    void getResultORFuzzy_(const std::vector<uint32_t>& termidList, std::vector<std::vector<pair<uint32_t, unsigned short> >* >& resultList)
    {
        std::vector<pair<uint32_t, unsigned short> >* v = NULL;
        uint32_t size;
        for (std::vector<uint32_t>::const_iterator i = termidList.begin(); i != termidList.end(); ++i)
        {
            getTermIdResult_(*i, v, size);
            resultList.push_back(v);
        }
    }

    void mergeAnd_(const std::vector<std::vector<pair<uint32_t, unsigned short> >* >& docidLists, std::vector<uint32_t>& resultList, std::vector<double>& ResultListSimilarity)// get add result; all the vector<docid> is sorted;
    {
        izenelib::util::ClockTimer timer;
        unsigned int size = docidLists.size();
        if (size == 0)
        {
            return;
        }
        unsigned int min = 0;
        unsigned int label = 0;
        for (unsigned int i = 0; i < size; ++i)
        {
            if (min == 0)
            {
                min = (*(docidLists[i])).size();
                label = i;
                continue;
            }
            if (min > (*(docidLists[i])).size())
            {
                min = (*(docidLists[i])).size();
                label = i;
            }
        }

        std::vector<pair<uint32_t, unsigned short> >::const_iterator iter = (*(docidLists[label])).begin();
        vector<int> iteratorList;
        for (std::vector<std::vector<pair<uint32_t, unsigned short> >* >::const_iterator i = docidLists.begin(); i != docidLists.end(); ++i)
        {
            iteratorList.push_back(0);
        }

        for ( ; iter != (*(docidLists[label])).end(); ++iter)
        {
            std::vector<std::vector<pair<uint32_t, unsigned short> >* >::const_iterator i = docidLists.begin();
            uint32_t count = 0;
            bool flag = true;
            while(count < size)
            {
                if (count == label)
                {
                    count++;
                    i++;
                    continue;
                }
                iteratorList[count] = BinSearch_(**i, iteratorList[count], (**i).size(), (*iter).first);//XXXOPTIM
                if( iteratorList[count] == -1)
                {
                    flag = false;
                    break;
                }
                i++;
                count++;
            }

            if(flag)
            {
                if (resultList.size() == 0)
                {
                    resultList.push_back((*iter).first);
                }
                if (resultList.size() > 0 && (*iter).first != resultList[resultList.size()-1])//may has the same
                {
                    resultList.push_back((*iter).first);
                }
            }
        }

        LOG(INFO)<<"mergeAnd and cost:"<<timer.elapsed()<<" seconds"<<endl;
    }

    void print()
    {
        cout<<"docidNumbers_:";
        unsigned int k = 0;
        for (uint32_t i = 0; i < termidList_.size(); ++i)
        {
            k += docidLists_[i].size();
        }
        cout<<k<<endl<<"termidList_.....:"<<termidList_.size()<<endl;
    }

    bool save_(string path = "")
    {
        string index_path = Increment_index_path_ + path;
        FILE *file;
        if ((file = fopen(index_path.c_str(), "wb")) == NULL)
        {
            LOG(INFO) << "Cannot open output file" << endl;
            return false;
        }
        unsigned int count = termidList_.size();
        fwrite(&count, sizeof(count), 1, file);
        for (std::set<std::pair<uint32_t, uint32_t>, pairLess>::iterator i = termidList_.begin(); i != termidList_.end(); ++i)
        {
            fwrite(&(*i), sizeof(pair<uint32_t, uint32_t>), 1, file);
        }
        fwrite(&allIndexDocNum_, sizeof(allIndexDocNum_), 1, file);
        for (std::vector<std::vector<pair<uint32_t, unsigned short> > >::iterator i = docidLists_.begin(); i != docidLists_.end(); ++i)
        {
            unsigned int size = (*i).size();
            fwrite(&size, sizeof(size), 1, file);
            for (std::vector<pair<uint32_t, unsigned short> >::iterator it = (*i).begin(); it != (*i).end(); ++it)
            {
                fwrite(&(*it), sizeof(pair<uint32_t, unsigned short>), 1, file);
            }
        }
        fclose(file);
        return true;
    }

    bool load_(string path = "")
    {
        string index_path = Increment_index_path_ + path;
        FILE *file;
        if ((file = fopen(index_path.c_str(), "rb")) == NULL)
        {
            LOG(INFO) << "Cannot open output file" << endl;
            return false;
        }
        unsigned int count = 0;
        unsigned int readSize = 0;
        readSize = fread(&count, sizeof(unsigned int), 1, file);
        if (readSize != 1)
        {
            LOG(INFO) <<"Fread Wrong!!"<<endl;
            return false;
        }

        for (unsigned int i = 0; i < count; ++i)
        {
            pair<uint32_t, uint32_t> doc;
            readSize = fread(&doc, sizeof(doc), 1, file);
            termidList_.insert(doc);
        }

        readSize = fread(&allIndexDocNum_, sizeof(allIndexDocNum_), 1, file);
        if (readSize != 1)
        {
            LOG(INFO) <<"Fread Wrong!!"<<endl;
            return false;
        }

        unsigned int i = 0;
        while( i < allIndexDocNum_)
        {
            unsigned int size = 0;
            std::vector<pair<uint32_t, unsigned short> > v;
            readSize = fread(&size, sizeof(count), 1, file);

            for (unsigned int j = 0; j < size; ++j)
            {
                pair<uint32_t, unsigned short> p;
                readSize = fread(&p, sizeof(pair<uint32_t, unsigned short>), 1, file);

                v.push_back(p);
                i++;
            }
            docidLists_.push_back(v);
            v.clear();
        }
        fclose(file);
        return true;
    }

    void prepare_index_()
    {
        std::vector<std::vector<pair<uint32_t, unsigned short> > >::iterator iter = docidLists_.begin();
        for (; iter != docidLists_.end(); ++iter)
        {
            sort((*iter).begin(), (*iter).end());
        }
    }
    void addTerm_(const pair<uint32_t, uint32_t> &termid, const std::vector<pair<uint32_t, unsigned short> >& docids)
    {
        std::set<std::pair<uint32_t, uint32_t>, pairLess>::const_iterator it = termidList_.find(termid);
        allIndexDocNum_ += docids.size();
        if (it != termidList_.end())
        {
            for (std::vector<pair<uint32_t, unsigned short> >::const_iterator iter = docids.begin(); iter != docids.end(); ++iter)
            {
                docidLists_[(*it).second].push_back(*iter);
            }
        }
        else
        {
            pair<uint32_t, uint32_t>  p(termid.first, docidLists_.size());
            std::vector<pair<uint32_t, unsigned short> > v;
            docidLists_.push_back(v);
            termidList_.insert(p);
            for (std::vector<pair<uint32_t, unsigned short> >::const_iterator iter = docids.begin(); iter != docids.end(); ++iter)
            {
                docidLists_[docidLists_.size()-1].push_back(*iter);
            }
        }
    }

    std::set<std::pair<uint32_t, uint32_t>, pairLess>& gettermidList_()
    {
        return termidList_;
    }

    std::vector<std::vector<pair<uint32_t, unsigned short> > >& getdocidLists_()
    {
        return docidLists_;
    }

    const string& getPath() const
    {
        return Increment_index_path_;
    }
private:
    int BinSearch_(std::vector<pair<unsigned int, unsigned short> >&A, int min, int max, unsigned int key)
    {
        while(min <= max)
        {
            int mid = (min + max)/2;
            if(A[mid].first == key)
                return mid;
            else if(A[mid].first > key)
                max = mid - 1;
            else
                min = mid + 1;
        }
        return -1;
    }

    std::string Increment_index_path_;

    std::vector<std::vector<pair<uint32_t, unsigned short> > > docidLists_;

    std::vector<unsigned int> docidNumbers_;

    std::set<std::pair<uint32_t, uint32_t>, pairLess> termidList_;

    unsigned int termidNum_;

    unsigned int allIndexDocNum_;

    unsigned int Max_Doc_Num_;//save and load.... here we will not add max_docid and start_id

    cma::Analyzer* analyzer_;
};

class IndexBarrel
{
public:
    IndexBarrel(
        const std::string& path,
        boost::shared_ptr<IDManager>& idManager,
        boost::shared_ptr<LAManager>& laManager,
        IndexBundleSchema& indexSchema,
        unsigned int Max_Doc_Num,
        cma::Analyzer* &analyzer);

    ~IndexBarrel()
    {
        if (pForwardIndex_ != NULL)
        {
            delete pForwardIndex_;
            pForwardIndex_ = NULL;
        }

        if (pIncrementIndex_ != NULL)
        {
            delete pIncrementIndex_;
            pIncrementIndex_ = NULL;
        }
    }

    void reset()
    {
        if (pForwardIndex_ != NULL)
        {
            delete pForwardIndex_;
            pForwardIndex_ = NULL;
        }

        if (pIncrementIndex_ != NULL)
        {
            delete pIncrementIndex_;
            pIncrementIndex_ = NULL;
        }
        path pathMainInc = doc_file_path_ + ".inc.idx";
        path pathMainFd = doc_file_path_ + ".fd.idx";

        boost::filesystem::remove(pathMainInc);
        boost::filesystem::remove(pathMainFd);
    }

    bool init(std::string& path)
    {
        pForwardIndex_ = new ForwardIndex(doc_file_path_, Max_Doc_Num_);
        pIncrementIndex_ = new IncrementIndex(doc_file_path_, Max_Doc_Num_, analyzer_);//
        if (pForwardIndex_ == NULL || pIncrementIndex_ == NULL)
        {
            return false;
        }
        return true;
    }

    bool load_(string path = "")
    {
        if (pForwardIndex_ != NULL && pIncrementIndex_ != NULL)
        {
            return pForwardIndex_->load_(path)&&pIncrementIndex_->load_(path);
        }
        else
        {
            LOG(INFO) << "Index is not exit!!"<<endl;
            return false;
        }
    }

    bool save_(string path = "")
    {
        if (pForwardIndex_ != NULL && pIncrementIndex_ != NULL)
        {
            if (isAddedIndex_ == true)
            {
                return pForwardIndex_->save_(path)&&pIncrementIndex_->save_(path);
            }
            return true;
        }
        else
        {
            LOG(INFO) << "Index is not exit!!"<<endl;
            return false;
        }
    }

    uint32_t getScore(std::vector<uint32_t> &v, uint32_t & docid)
    {
        if (pForwardIndex_)
        {
            return pForwardIndex_->getSorce(v, docid);
        }
        return 0;
    }

    void setStatus()
    {
        isAddedIndex_ = true;
    }

    void resetStatus()
    {
        isAddedIndex_ = false;
    }

    bool buildIndex_(uint32_t docId, std::string& text);
    bool score(const std::string& query, std::vector<uint32_t>& resultList, std::vector<double> &ResultListSimilarity);

    void getFuzzyResult_(std::vector<uint32_t>& termidList, std::vector<uint32_t>& resultList,  std::vector<double>& ResultListSimilarity, uint32_t& hitdoc)//
    {

        std::vector<pair<uint32_t, unsigned short> >* v = NULL;
        pForwardIndex_->resetBitMapMatrix();

        izenelib::util::ClockTimer timerx;
        for (std::vector<uint32_t>::iterator i = termidList.begin(); i != termidList.end(); ++i)
        {
            pIncrementIndex_->getTermIdPos_(*i, v);
            if (v == NULL)
                return;
            pForwardIndex_->setPosition_(v);
        }

        std::vector<std::vector<pair<uint32_t, unsigned short> >* > ORResultList;
        pIncrementIndex_->getResultORFuzzy_(termidList, ORResultList);

        std::vector<uint32_t> newTermidList;
        pForwardIndex_->getLongestSubString_(newTermidList, ORResultList, hitdoc);

        std::vector<uint32_t >::iterator iter = std::unique(newTermidList.begin(), newTermidList.end());
        newTermidList.erase(iter, newTermidList.end());
        pIncrementIndex_->getResultAND_(newTermidList, resultList, ResultListSimilarity);
    }

    void getExactResult_(std::vector<uint32_t>& termidList, std::vector<uint32_t>& resultList,  std::vector<double>& ResultListSimilarity)
    {
        izenelib::util::ClockTimer timer;
        pIncrementIndex_->getResultAND_(termidList, resultList, ResultListSimilarity);
        LOG(INFO)<<"ResultList Number:"<<resultList.size()<< " time cost:"<< timer.elapsed()<<endl;
    }

    void print()
    {
        pForwardIndex_->print();
        pIncrementIndex_->print();
    }

    void prepare_index_()
    {
        pIncrementIndex_->prepare_index_();
    }

    ForwardIndex* getForwardIndex()
    {
        return pForwardIndex_;
    }

    IncrementIndex* getIncrementIndex()
    {
        return pIncrementIndex_;
    }
private:
    ForwardIndex* pForwardIndex_;

    IncrementIndex* pIncrementIndex_;

    boost::shared_ptr<IDManager> idManager_;

    boost::shared_ptr<LAManager> laManager_;

    IndexBundleSchema& indexSchema_;

    unsigned int DocNumber_;

    bool isAddedIndex_;

    std::string doc_file_path_;

    unsigned int Max_Doc_Num_;

    cma::Analyzer* analyzer_;
};

class IncrementalManager
{
public:
    IncrementalManager(const std::string& path,
                       const std::string& tokenize_path,
                       const std::string& property,
                       boost::shared_ptr<DocumentManager>& document_manager,
                       boost::shared_ptr<IDManager>& idManager,
                       boost::shared_ptr<LAManager>& laManager,
                       IndexBundleSchema& indexSchema
                      );

    ~IncrementalManager()
    {
        if (pMainBarrel_ != NULL)
        {
            delete pMainBarrel_;
            pMainBarrel_ = NULL;
        }
        if (pTmpBarrel_!= NULL)
        {
            delete pTmpBarrel_;
            pTmpBarrel_ = NULL;
        }
        if (analyzer_)
        {
            //delete analyzer_;
            analyzer_ = NULL;
        }
        if (knowledge_)
        {
            //delete knowledge_;
            analyzer_ = NULL;
        }
    }

    void startIncrementalManager()
    {
        bool flag = true;
        std::string pathMainInc = index_path_ + "/Main.inc.idx";
        std::string pathMainFd = index_path_ + "/Main.fd.idx";
        std::string pathTmpInc = index_path_ + "/Tmp.inc.idx";
        std::string pathTmpFd = index_path_ + "/Tmp.fd.idx";
        std::string pathLastDocid = index_path_ + "/last.docid";

        if (boost::filesystem::exists(pathMainInc) && boost::filesystem::exists(pathMainFd))//main
        {
            init_();
            if(!pMainBarrel_->load_())
            {
                LOG(INFO) << "Index Wrong!!"<<endl;
                delete_AllIndexFile();
                flag = false;
            }
            else
            {
                if (boost::filesystem::exists(pathTmpInc) && boost::filesystem::exists(pathTmpFd))//tmp
                {
                    init_tmpBerral();
                    if(!pTmpBarrel_->load_())
                    {
                        LOG(INFO) << "Index Wrong!!"<<endl;
                        delete_AllIndexFile();
                        flag = false;
                    }
                }
            }
        }
        else
        {
            flag = false;
            loadLastDocid_();
        }
        if (flag)
        {
            if(loadLastDocid_())
            {
                isStartFromLocal_ = true;
            }
        }
        else
        {
            if (!isStartFromLocal_)
            {
                if (pMainBarrel_ != NULL)
                {
                    delete pMainBarrel_;
                    pMainBarrel_ = NULL;
                }
                if (pTmpBarrel_!= NULL)
                {
                    delete pTmpBarrel_;
                    pTmpBarrel_ = NULL;
                }
                delete_AllIndexFile();
            }
        }
    }

    bool init_()
    {
        if (pMainBarrel_ == NULL)
        {
            string path = index_path_ + "/Main";
            pMainBarrel_ = new IndexBarrel(
                path,
                idManager_,
                laManager_,
                indexSchema_,
                MAX_INCREMENT_DOC,
                analyzer_
            );
            BarrelNum_++;
            if (pMainBarrel_ == NULL)
            {
                BarrelNum_--;
                return false;
            }
            pMainBarrel_->init(index_path_);
        }
        return true;
    }

    bool init_tmpBerral()
    {
        if (pTmpBarrel_ == NULL)
        {
            BarrelNum_++;
            string path = index_path_ + "/Tmp";
            pTmpBarrel_ = new IndexBarrel(
                path,
                idManager_,
                laManager_,
                indexSchema_,
                MAX_TMP_DOC,
                analyzer_
            );
            if (pTmpBarrel_ == NULL)
            {
                BarrelNum_--;
                return false;
            }
            pTmpBarrel_->init(index_path_);
        }
        return true;
    }

    void InitManager_();

    unsigned int edit_distance(const string& s1, const string& s2)
    {
        const size_t len1 = s1.size(), len2 = s2.size();
        vector<vector<unsigned int> > d(len1 + 1, vector<unsigned int>(len2 + 1));

        d[0][0] = 0;
        for(unsigned int i = 1; i <= len1; ++i) d[i][0] = i;
        for(unsigned int i = 1; i <= len2; ++i) d[0][i] = i;

        for(unsigned int i = 1; i <= len1; ++i)
            for(unsigned int j = 1; j <= len2; ++j)

                d[i][j] = std::min( std::min(d[i - 1][j] + 1,d[i][j - 1] + 1),
                                    d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1) );
        return d[len1][len2];
    }

    void createIndex_();

    bool index_(uint32_t& docId, std::string propertyString)
    {

        if(IndexedDocNum_ >= MAX_INCREMENT_DOC)
            return false;
        if ( isInitIndex_ == false)
        {
            init_tmpBerral();
            {
                if (pTmpBarrel_ != NULL)
                {
                    pTmpBarrel_->setStatus();
                    if(!pTmpBarrel_->buildIndex_(docId, propertyString))
                        return false;
                }
                IndexedDocNum_++;
            }
        }
        else
        {
            {
                if (pMainBarrel_ != NULL)
                {
                    pMainBarrel_->setStatus();
                    if(!pMainBarrel_->buildIndex_(docId, propertyString))
                        return false;
                }
                IndexedDocNum_++;
            }
        }
        return true;
    }

    void delete_AllIndexFile()
    {
        path pathMainInc = index_path_ + "/Main.inc.idx";
        path pathMainFd = index_path_ + "/Main.fd.idx";
        path pathTmpInc = index_path_ + "/Tmp.inc.idx";
        path pathTmpFd = index_path_ + "/Tmp.fd.idx";

        boost::filesystem::remove(pathMainInc);
        boost::filesystem::remove(pathMainFd);
        boost::filesystem::remove(pathTmpInc);
        boost::filesystem::remove(pathTmpFd);
    }

    bool fuzzySearch_(const std::string& query, std::vector<uint32_t>& resultList, std::vector<double> &ResultListSimilarity);

    bool exactSearch_(const std::string& query, std::vector<uint32_t>& resultList, std::vector<double> &ResultListSimilarity);

    void doCreateIndex_();

    void mergeIndex();

    void buildTokenizeDic();

    void setLastDocid(uint32_t last_docid);

    void getLastDocid(uint32_t& last_docid)
    {
        last_docid = last_docid_;
    }

    void getDocNum(uint32_t& docNum)
    {
        docNum = IndexedDocNum_;
    }

    void getMaxNum(uint32_t& maxNum)
    {
        maxNum = MAX_INCREMENT_DOC;
    }

    void prepare_index_()
    {
        if (pMainBarrel_)
        {
            pMainBarrel_->prepare_index_();
        }
        if (pTmpBarrel_)
        {
            pTmpBarrel_->prepare_index_();
        }
    }

    void reset()
    {
        if(pMainBarrel_)
        {
            pMainBarrel_->reset();
            delete pMainBarrel_;
            pMainBarrel_ = NULL;
            BarrelNum_ = 0;
            IndexedDocNum_ = 0;
        }
    }

    void save_()
    {
        if (pMainBarrel_)
        {
            pMainBarrel_->save_();
        }
    }

    bool saveLastDocid_(string path = "")
    {
        string docid_path = index_path_ + "/last.docid" + path;
        FILE* file;
        if ((file = fopen(docid_path.c_str(), "wb")) == NULL)
        {
            LOG(INFO) << "Cannot open output file"<<endl;
            return false;
        }
        fwrite(&last_docid_, sizeof(last_docid_), 1, file);
        fwrite(&IndexedDocNum_, sizeof(IndexedDocNum_), 1, file);
        fclose(file);
        return true;
    }

    bool loadLastDocid_(string path = "")
    {
        string docid_path = index_path_ + "/last.docid" + path;
        FILE* file;
        if ((file = fopen(docid_path.c_str(), "rb")) == NULL)
        {
            LOG(INFO) << "Cannot open input file"<<endl;
            return false;
        }
        if (1 != fread(&last_docid_, sizeof(last_docid_), 1, file) ) return false;
        if (1 != fread(&IndexedDocNum_, sizeof(IndexedDocNum_), 1, file) ) return false;
        fclose(file);
        return true;
    }

private:
    uint32_t last_docid_;

    std::string index_path_;

    std::string tokenize_path_;

    std::string property_;

    unsigned int BarrelNum_;

    IndexBarrel* pMainBarrel_;

    IndexBarrel* pTmpBarrel_;

    unsigned int IndexedDocNum_;

    bool isMergingIndex_;

    bool isInitIndex_;

    bool isAddingIndex_;

    bool isStartFromLocal_;

    boost::shared_ptr<DocumentManager> document_manager_;

    boost::shared_ptr<IDManager> idManager_;

    boost::shared_ptr<LAManager> laManager_;

    IndexBundleSchema indexSchema_;

    cma::Analyzer* analyzer_;

    cma::Knowledge* knowledge_;

    typedef boost::shared_mutex MutexType;
    typedef boost::shared_lock<MutexType> ScopedReadLock;
    typedef boost::unique_lock<MutexType> ScopedWriteLock;
    mutable MutexType mutex_;
};
}