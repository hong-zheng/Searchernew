#pragma once

#include "cppjieba/Jieba.hpp"
#include <iostream>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <jsoncpp/json/json.h>
using namespace std ;

namespace searcher{
///////////////////////////////////////////////////////////
// 接下来代码是索引模块内容
///////////////////////////////////////////////////////////

// 先定义一个基本的索引中需要用到的结构
// 这个结构是正排索引结构
// 正排索引是给定 doc_id 映射到文档内容(DocInfo对象）
struct DocInfo{
	int64_t doc_id ;
	string title ;
	string url ;
	string content ;
} ;


// 倒排索引是给定词，映射到包含该词的文档 id 列表.(此处不光要有文档 id)
// 还得有权重信息，以及该词的内容）
struct Weight{
	// 该词在哪一个文档中出现
	int64_t doc_id ;
	// 对应的权重是多少
	int weight ;
	// 词是啥
	string word ;
} ;
// 倒排拉链
typedef vector<Weight> InvertedList ;
// Index 类用于表示真个索引结构，并且提供一些 API 供外部调用
class Index{
private:
	// 索引结构
	// 正排索引：数组下表对应到 doc_id 
	vector<DocInfo> forward_index ;
	// 倒排索引，使用一个 hash 表来表示这个影射关系
//	unordered_map<string,vector<Weight>> inverted_index ;
	unordered_map<string,InvertedList> inverted_index ;

public:
	Index() ;
	// 提供一些对外调用的函数
	// 1、查正排:返回指针就可以使用 NULL 表示无效结果
	const DocInfo* GetDocInfo(int64_t doc_id) ;
	// 2、查倒排
	const InvertedList* GetInvertedList(const string& key) ;
	// 3、构建索引
	bool Build(const string& input_path) ;
	void CutWord(const string& input , vector<string>* output) ;	
private:
	DocInfo* BuildForward(const string& line) ;
	void BuildInverted(const DocInfo& doc_info) ;

	cppjieba::Jieba jieba ;
} ;
///////////////////////////////////////////////////////////
// 接下来这个模块是搜索模块
///////////////////////////////////////////////////////////
class Searcher{
private:
	// 搜索过程依赖索引，就需要持有一个 Index 的指针
	Index* index ;
public:
	Searcher() : index(new Index()){}

	bool Init(const string& input_path) ;
	bool Search(const string& query , string* results ) ;
private:
	string GenerateDesc(const string& content ,const string& word) ;
} ; 

} // namespace searcher
