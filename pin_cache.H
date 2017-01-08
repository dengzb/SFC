/*BEGIN_LEGAL
Intel Open Source License

Copyright (c) 2002-2016 Intel Corporation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
//
// @ORIGINAL_AUTHOR: Artur Klauser
//

/*! @file
 *  This file contains a configurable cache class
 */

#ifndef PIN_CACHE_H
#define PIN_CACHE_H

#include <string>
#include <vector>

#include "pin_util.H"

/*!
 *  @brief Checks if n is a power of 2.
 *  @returns true if n is power of 2
 */
static inline bool IsPower2(UINT32 n)
{
    return ((n & (n - 1)) == 0);
}

/*!
 *  @brief Computes floor(log2(n))
 *  Works by finding position of MSB set.
 *  @returns -1 if n == 0.
 */
static inline INT32 FloorLog2(UINT32 n)
{
    INT32 p = 0;

    if (n == 0) return -1;

    if (n & 0xffff0000) { p += 16; n >>= 16; }
    if (n & 0x0000ff00)	{ p +=  8; n >>=  8; }
    if (n & 0x000000f0) { p +=  4; n >>=  4; }
    if (n & 0x0000000c) { p +=  2; n >>=  2; }
    if (n & 0x00000002) { p +=  1; }

    return p;
}

/*!
 *  @brief Computes floor(log2(n))
 *  Works by finding position of MSB set.
 *  @returns -1 if n == 0.
 */
static inline INT32 CeilLog2(UINT32 n)
{
    return FloorLog2(n - 1) + 1;
}

/*!
 *  @brief Cache tag - self clearing on creation
 */
class CACHE_TAG
{
  private:
    ADDRINT _tag;

  public:
    CACHE_TAG(ADDRINT tag = 0) { _tag = tag; }
    bool operator==(const CACHE_TAG &right) const { return _tag == right._tag; }
    operator ADDRINT() const { return _tag; }
};
//! @my tag class

class myTag
{
private:
    ADDRINT _tag;
    int _cs;//0:uncompressed,1:compressed
    UINT32 _csize;//unit: segment
public:
    int cs(){return _cs;}
    UINT32 csize(){return _csize;}
    myTag(ADDRINT tag = 0,int state = 0,int csize = 0){_tag=tag;_csize=csize;_cs=state;}
    //myTag(myTag t){this->_tag=(ADDRINT)t;this->_csize=t.csize();this->_cs=t.cs();this->_start=t.start();}
    myTag(CACHE_TAG tag){_tag=tag;_csize=8;_cs=0;}//initialization

    bool operator==(const myTag &right)const{return _tag==right._tag;}//overwrite operator ==
    operator ADDRINT()  const{return _tag;}// overwrite type caster operator (ADDRINT)


    void setcs(int cs){_cs=cs;}
    void setcsize(UINT32 csize){_csize=csize;}

};

/*!
 * Everything related to cache sets
 */
namespace CACHE_SET
{

/*!
 *  @brief Cache set direct mapped
 */
class DIRECT_MAPPED
{
  private:
    CACHE_TAG _tag;

  public:
    DIRECT_MAPPED(UINT32 associativity = 1) { ASSERTX(associativity == 1); }

    VOID SetAssociativity(UINT32 associativity) { ASSERTX(associativity == 1); }
    UINT32 GetAssociativity(UINT32 associativity) { return 1; }

    UINT32 Find(CACHE_TAG tag) { return(_tag == tag); }
    VOID Replace(CACHE_TAG tag) { _tag = tag; }
    VOID Flush() { _tag = 0; }
};

/*!
 *  @brief Cache set with round robin replacement
 */
template <UINT32 MAX_ASSOCIATIVITY = 4>
class ROUND_ROBIN
{
  private:
    CACHE_TAG _tags[MAX_ASSOCIATIVITY];
    UINT32 _tagsLastIndex;
    UINT32 _nextReplaceIndex;

  public:
    ROUND_ROBIN(UINT32 associativity = MAX_ASSOCIATIVITY)
      : _tagsLastIndex(associativity - 1)
    {
        ASSERTX(associativity <= MAX_ASSOCIATIVITY);
        _nextReplaceIndex = _tagsLastIndex;

        for (INT32 index = _tagsLastIndex; index >= 0; index--)
        {
            _tags[index] = CACHE_TAG(0);
        }
    }

    VOID SetAssociativity(UINT32 associativity)
    {
        ASSERTX(associativity <= MAX_ASSOCIATIVITY);
        _tagsLastIndex = associativity - 1;
        _nextReplaceIndex = _tagsLastIndex;
    }
    UINT32 GetAssociativity(UINT32 associativity) { return _tagsLastIndex + 1; }

    UINT32 Find(CACHE_TAG tag)
    {
        bool result = true;

        for (INT32 index = _tagsLastIndex; index >= 0; index--)
        {
            // this is an ugly micro-optimization, but it does cause a
            // tighter assembly loop for ARM that way ...
            if(_tags[index] == tag) goto end;
        }
        result = false;

        end: return result;
    }

    VOID Replace(CACHE_TAG tag)
    {
        // g++ -O3 too dumb to do CSE on following lines?!
        const UINT32 index = _nextReplaceIndex;

        _tags[index] = tag;
        // condition typically faster than modulo
        _nextReplaceIndex = (index == 0 ? _tagsLastIndex : index - 1);
    }
    VOID Flush()
    {
      for (INT32 index = _tagsLastIndex; index >= 0; index--)
      {
	_tags[index] = 0;
      }
      _nextReplaceIndex=_tagsLastIndex;
    }
};







template<UINT32 ASSOCIATIVITY = 4, UINT32 BLOCKSIZE = 64>
class LRU
{
private:
    UINT32 _Max_Associativity;
    int _num_segment;//data area of this set, every segment has 8byte space,
    myTag _que[8*ASSOCIATIVITY];
    int _lru[8*ASSOCIATIVITY];
    UINT32 _restspace;
    UINT32 _Block_size;//byte1
    UINT32 _segsize;//byte
    UINT32 _Associativity;

public:

    LRU()
    {
        _Block_size=BLOCKSIZE;
        _restspace=32;
        _Associativity=ASSOCIATIVITY;
        _segsize=8;//byte
        _Max_Associativity = 8*_Associativity;
        _num_segment=_Associativity*_Block_size/_segsize;
        for(UINT32 i=0;i<_Max_Associativity;i++){
            _que[i]=myTag();
        }
        for(UINT32 i=0;i<_Max_Associativity;i++){
            _lru[i]=0;
        }
    }

    VOID SetAssociativity(UINT32 associativity) { return; }
    UINT32 GetAssociativity(UINT32 associativity) { return _Associativity; }


/*
    UINT32 getSize(){
        UINT32 Size=0;
        for(UINT32 i=0;i<_Max_Associativity;i++){
            if(0!=_que[i])
                Size++;
        }
        return Size;
    }
*/
    int Find(myTag tag){
        for(UINT32 i=0;i<_Max_Associativity;i++){
            if (tag==_que[i]){
                return (int)i;
            }
        }
        return -1;
    }

    int Find(){
        for(UINT32 i=0;i<_Max_Associativity;i++){
            if (0==_que[i]){
                return (int)i;
            }
        }
        return -1;
    }

    void updateLRU(UINT32 p){
        for(int i=0;(UINT32)i<_Max_Associativity;i++){
            if(_lru[i]!=0&&_lru[i]<_lru[p]) _lru[i]++;
        }
        _lru[p]=1;



    }

    int findLRU(){
        int Max=0;
        for(int i=0;(UINT32)i<_Max_Associativity;i++){
            if(_lru[i]>_lru[Max]) Max=i;
        }
        return Max;

    }


    bool FindSpace(myTag tag)  //find the position that can insert new tag
    {
        //int space=0;
        //int Size=tag.csize();
        //for(UINT32 i=0;i<_que.size();i++){
         //   space+=_que.at(i).csize();
        ///}
        return _restspace>tag.csize();

    }

    UINT32 compress(ADDRINT addr){//return the coompressed cache line length

        UINT32 actuallSize=0;//bits
        UINT32 *p=(UINT32 *)addr;
        //for(UINT32 i=0;i<2;i++,(p+=_Block_size/4))
        //{

            UINT32 t1=*p;

            if((t1>>4)==0||(~t1>>4)==0) actuallSize+=4;//4 bits extent
            else if((t1>>8)==0||(~t1>>8)==0) actuallSize+=8;//8 bits extent
            else if((t1>>16)==0||(~t1>>16)==0) actuallSize+=16;// half word extent



            else actuallSize+=32;
        //}
            //if(actuallSize=2048) actuallSize=2047;

        //return actuallSize/64;//transform the bits to segment

        return actuallSize/32*_num_segment;
        //return 8;

        }


   /* int Remove(myTag tag)                  //remove the tag from tag area and data area
    {
        int temp= Find(tag);
        if (temp==-1) return -1;
        //_que.erase(_que.begin()+temp);     //remove from the tag area
        for(UINT32 i=0;i<_que.size();i++){ //remove from the lru que
            if (tag==_lru.at(i)){
                _lru.erase(_lru.begin()+i);
                break;
            }
        }
                     //remove from the LRU
        _que[temp]=myTag();

        return 1;
    }
*/


    void Replace(ADDRINT tag, ADDRINT addr)//version0.1 only consider the situation not hit
    {
            /*
            myTag newtag=myTag(tag);
            int realSize=compress(newtag,addr);
            newtag.setcs(realSize==8?0:1);
            newtag.setcsize(realSize);
            while(!FindSpace(newtag)){
                Remove(_lru.back());
            }
            _que.push_back(newtag);
            _lru.insert(0,newtag);
            */
            //_que.pop_back();
            //_que.insert(0,tag);
            myTag newtag = myTag(tag);
            newtag.setcsize(compress(addr));
            newtag.setcs(newtag.csize()==8?1:0);
            int p=0;
        while(!FindSpace(newtag)){
            p=findLRU();
            _restspace+=_que[p].csize();
            _que[p]=myTag();
            _lru[p]=0;
        }
        if(FindSpace(newtag)) p=Find();
        _que[p]=newtag;
        _restspace-=newtag.csize();
        updateLRU(p);
        return;
    }
    //when a write hit happen but the new value has the different size, it need to be replaced
    void hitReplace(myTag &tag)
    {

        int p = Find(tag);
        updateLRU(p);
        return;
    }


};

} // namespace CACHE_SET

namespace CACHE_ALLOC
{
    typedef enum
    {
        STORE_ALLOCATE,
        STORE_NO_ALLOCATE
    } STORE_ALLOCATION;
}

/*!
 *  @brief Generic cache base class; no allocate specialization, no cache set specialization
 */
class CACHE_BASE
{
  public:
    // types, constants
    typedef enum
    {
        ACCESS_TYPE_LOAD,
        ACCESS_TYPE_STORE,
        ACCESS_TYPE_NUM
    } ACCESS_TYPE;

  protected:
    static const UINT32 HIT_MISS_NUM = 2;
    CACHE_STATS _access[ACCESS_TYPE_NUM][HIT_MISS_NUM];

  private:
    // input params
    const std::string _name;
    const UINT32 _cacheSize;
    const UINT32 _lineSize;
    const UINT32 _associativity;
    UINT32 _numberOfFlushes;
    UINT32 _numberOfResets;

    // computed params
    const UINT32 _lineShift;
    const UINT32 _setIndexMask;

    CACHE_STATS SumAccess(bool hit) const
    {
        CACHE_STATS sum = 0;

        for (UINT32 accessType = 0; accessType < ACCESS_TYPE_NUM; accessType++)
        {
            sum += _access[accessType][hit];
        }

        return sum;
    }

  protected:
    UINT32 NumSets() const { return _setIndexMask + 1; }

  public:
    // constructors/destructors
    CACHE_BASE(std::string name, UINT32 cacheSize, UINT32 lineSize, UINT32 associativity);

    // accessors
    UINT32 CacheSize() const { return _cacheSize; }
    UINT32 LineSize() const { return _lineSize; }
    UINT32 Associativity() const { return _associativity; }
    //
    CACHE_STATS Hits(ACCESS_TYPE accessType) const { return _access[accessType][true];}
    CACHE_STATS Misses(ACCESS_TYPE accessType) const { return _access[accessType][false];}
    CACHE_STATS Accesses(ACCESS_TYPE accessType) const { return Hits(accessType) + Misses(accessType);}
    CACHE_STATS Hits() const { return SumAccess(true);}
    CACHE_STATS Misses() const { return SumAccess(false);}
    CACHE_STATS Accesses() const { return Hits() + Misses();}

    CACHE_STATS Flushes() const { return _numberOfFlushes;}
    CACHE_STATS Resets() const { return _numberOfResets;}

    VOID SplitAddress(const ADDRINT addr, CACHE_TAG & tag, UINT32 & setIndex) const
    {
        tag = addr >> _lineShift;
        setIndex = tag & _setIndexMask;
    }

    VOID SplitAddress(const ADDRINT addr, CACHE_TAG & tag, UINT32 & setIndex, UINT32 & lineIndex) const
    {
        const UINT32 lineMask = _lineSize - 1;
        lineIndex = addr & lineMask;
        SplitAddress(addr, tag, setIndex);
    }

    VOID IncFlushCounter()
    {
	_numberOfFlushes += 1;
    }

    VOID IncResetCounter()
    {
	_numberOfResets += 1;
    }

    std::ostream & StatsLong(std::ostream & out) const;
};

CACHE_BASE::CACHE_BASE(std::string name, UINT32 cacheSize, UINT32 lineSize, UINT32 associativity)
  : _name(name),
    _cacheSize(cacheSize),
    _lineSize(lineSize),
    _associativity(associativity),
    _lineShift(FloorLog2(lineSize)),
    _setIndexMask((cacheSize / (associativity * lineSize)) - 1)
{

    ASSERTX(IsPower2(_lineSize));
    ASSERTX(IsPower2(_setIndexMask + 1));

    for (UINT32 accessType = 0; accessType < ACCESS_TYPE_NUM; accessType++)
    {
        _access[accessType][false] = 0;
        _access[accessType][true] = 0;
    }
}

/*!
 *  @brief Stats output method
 */
std::ostream & CACHE_BASE::StatsLong(std::ostream & out) const
{
    const UINT32 headerWidth = 19;
    const UINT32 numberWidth = 10;

    out << _name << ":" << std::endl;

    for (UINT32 i = 0; i < ACCESS_TYPE_NUM; i++)
    {
        const ACCESS_TYPE accessType = ACCESS_TYPE(i);

        std::string type(accessType == ACCESS_TYPE_LOAD ? "Load" : "Store");

        out << StringString(type + " Hits:      ", headerWidth)
            << StringInt(Hits(accessType), numberWidth) << std::endl;
        out << StringString(type + " Misses:    ", headerWidth)
            << StringInt(Misses(accessType), numberWidth) << std::endl;
        out << StringString(type + " Accesses:  ", headerWidth)
            << StringInt(Accesses(accessType), numberWidth) << std::endl;
        out << StringString(type + " Miss Rate: ", headerWidth)
            << StringFlt(100.0 * Misses(accessType) / Accesses(accessType), 2, numberWidth-1) << "%" << std::endl;

        out << std::endl;
    }

    out << StringString("Total Hits:      ", headerWidth, ' ')
        << StringInt(Hits(), numberWidth) << std::endl;
    out << StringString("Total Misses:    ", headerWidth, ' ')
        << StringInt(Misses(), numberWidth) << std::endl;
    out << StringString("Total Accesses:  ", headerWidth, ' ')
        << StringInt(Accesses(), numberWidth) << std::endl;
    out << StringString("Total Miss Rate: ", headerWidth, ' ')
        << StringFlt(100.0 * Misses() / Accesses(), 2, numberWidth-1) << "%" << std::endl;

    out << StringString("Flushes:         ", headerWidth, ' ')
        << StringInt(Flushes(), numberWidth) << std::endl;
    out << StringString("Stat Resets:     ", headerWidth, ' ')
        << StringInt(Resets(), numberWidth) << std::endl;
    out << std::endl;
    return out;
}

/// ostream operator for CACHE_BASE
std::ostream & operator<< (std::ostream & out, const CACHE_BASE & cacheBase)
{
    return cacheBase.StatsLong(out);
}

/*!
 *  @brief Templated cache class with specific cache set allocation policies
 *
 *  All that remains to be done here is allocate and deallocate the right
 *  type of cache sets.
 */
template <class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION>
class CACHE : public CACHE_BASE
{
  private:
    SET _sets[MAX_SETS];

  public:
    // constructors/destructors
    CACHE(std::string name, UINT32 cacheSize, UINT32 lineSize, UINT32 associativity)
      : CACHE_BASE(name, cacheSize, lineSize, associativity)
    {
        ASSERTX(NumSets() <= MAX_SETS);

        for (UINT32 i = 0; i < NumSets(); i++)
        {
            _sets[i].SetAssociativity(associativity);
        }
    }

    // modifiers
    /// Cache access from addr to addr+size-1
    bool Access(ADDRINT addr, UINT32 size, ACCESS_TYPE accessType);
    /// Cache access at addr that does not span cache lines
    bool AccessSingleLine(ADDRINT addr, ACCESS_TYPE accessType);
    bool SpecialAccess(ADDRINT addr, UINT32 size, ACCESS_TYPE accessType);
    bool SpecialAccessSingleLine(ADDRINT addr, ACCESS_TYPE accessType);
    void Flush();
    void ResetStats();
    //UINT32 geteSize();
};
/*
template <class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION>
UINT32 CACHE<SET,MAX_SETS,STORE_ALLOCATION>::geteSize(){
    UINT32 eff=0;
    for (UINT32 i=0;i<MAX_SETS;i++){
        eff+=_sets[i].getSize();
    }
    return eff;
}





*/
/*!
 *  @return true if all accessed cache lines hit
 */
template <class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION>
bool CACHE<SET,MAX_SETS,STORE_ALLOCATION>::Access(ADDRINT addr, UINT32 size, ACCESS_TYPE accessType)
{
    const ADDRINT highAddr = addr + size;
    bool allHit = true;

    const ADDRINT lineSize = LineSize();
    const ADDRINT notLineMask = ~(lineSize - 1);
    do
    {
        CACHE_TAG tag;
        UINT32 setIndex;

        SplitAddress(addr, tag, setIndex);

        SET & set = _sets[setIndex];

        bool localHit = set.Find(tag);
        allHit &= localHit;

        // on miss, loads always allocate, stores optionally
        if ( (! localHit) && (accessType == ACCESS_TYPE_LOAD || STORE_ALLOCATION == CACHE_ALLOC::STORE_ALLOCATE))
        {
            set.Replace(tag);
        }

        addr = (addr & notLineMask) + lineSize; // start of next cache line
    }
    while (addr < highAddr);

    _access[accessType][allHit]++;

    return allHit;
}

/*!
 *  @return true if accessed cache line hits
 */
template <class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION>
bool CACHE<SET,MAX_SETS,STORE_ALLOCATION>::AccessSingleLine(ADDRINT addr, ACCESS_TYPE accessType)
{
    CACHE_TAG tag;
    UINT32 setIndex;

    SplitAddress(addr, tag, setIndex);

    SET & set = _sets[setIndex];

    bool hit = set.Find(tag);

    // on miss, loads always allocate, stores optionally
    if ( (! hit) && (accessType == ACCESS_TYPE_LOAD || STORE_ALLOCATION == CACHE_ALLOC::STORE_ALLOCATE))
    {
        set.Replace(tag);
    }

    _access[accessType][hit]++;

    return hit;
}
/*!
 *  @return true if accessed cache line hits
 */
template <class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION>
void CACHE<SET,MAX_SETS,STORE_ALLOCATION>::Flush()
{
    for (INT32 index = NumSets(); index >= 0; index--) {
      SET & set = _sets[index];
      set.Flush();
    }
    IncFlushCounter();
}

template <class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION>
void CACHE<SET,MAX_SETS,STORE_ALLOCATION>::ResetStats()
{
    for (UINT32 accessType = 0; accessType < ACCESS_TYPE_NUM; accessType++)
    {
	_access[accessType][false] = 0;
	_access[accessType][true] = 0;
    }
    IncResetCounter();
}

//!    @my access method
template <class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION>
bool CACHE<SET,MAX_SETS,STORE_ALLOCATION>::SpecialAccess(ADDRINT addr, UINT32 size, ACCESS_TYPE accessType)
{
    const ADDRINT highAddr = addr + size;
    bool allHit = true;

    const ADDRINT lineSize = LineSize();
    const ADDRINT notLineMask = ~(lineSize - 1);
    do
    {
        CACHE_TAG temptag;
        UINT32 setIndex;

        SplitAddress(addr, temptag, setIndex);

        SET & set = _sets[setIndex];
        myTag tag=myTag(temptag);
        bool localHit = (set.Find(tag)!=-1);
        allHit &= localHit;

        // on miss, loads always allocate, stores optionally
        if ( (! localHit)/* && (accessType == ACCESS_TYPE_LOAD || STORE_ALLOCATION == CACHE_ALLOC::STORE_ALLOCATE)*/)
        {
            set.Replace(tag,addr);
        }
        else if(localHit){
            set.hitReplace(tag);
        }
        addr = (addr & notLineMask) + lineSize; // start of next cache line
    }
    while (addr < highAddr);

    _access[accessType][allHit]++;

    return allHit;
}

/*!
 *  @return true if accessed cache line hits
 */
template <class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION>
bool CACHE<SET,MAX_SETS,STORE_ALLOCATION>::SpecialAccessSingleLine(ADDRINT addr, ACCESS_TYPE accessType)
{
    CACHE_TAG temptag;
    UINT32 setIndex;

    SplitAddress(addr, temptag, setIndex);
    myTag tag=myTag(temptag);
    SET & set = _sets[setIndex];

    bool hit = (set.Find(tag)!=-1);

    // on miss, loads always allocate, stores optionally
    if ( (! hit) /*&& (accessType == ACCESS_TYPE_LOAD || STORE_ALLOCATION == CACHE_ALLOC::STORE_ALLOCATE)*/)
    {
        set.Replace(tag,addr);
    }
    else if(hit){
        set.hitReplace(tag);
    }
    _access[accessType][hit]++;

    return hit;
}

// define shortcuts
#define CACHE_DIRECT_MAPPED(MAX_SETS, ALLOCATION) CACHE<CACHE_SET::DIRECT_MAPPED, MAX_SETS, ALLOCATION>
#define CACHE_ROUND_ROBIN(MAX_SETS, MAX_ASSOCIATIVITY, ALLOCATION) CACHE<CACHE_SET::ROUND_ROBIN<MAX_ASSOCIATIVITY>, MAX_SETS, ALLOCATION>
#define CACHE_LRU(MAX_SETS,ASSOCIATIVITY,BLOCKSIZE,ALLOCATION) CACHE<CACHE_SET::LRU<ASSOCIATIVITY,BLOCKSIZE>,MAX_SETS,ALLOCATION>
#endif // PIN_CACHE_H
