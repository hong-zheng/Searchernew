#include "searcher.h"
#include "../common/util.hpp"

namespace searcher{


const char* const DICT_PATH = "/tmp/Tools/jieba_dict/dict/jieba.dict.utf8";
const char* const HMM_PATH = "/tmp/Tools/jieba_dict/dict/hmm_model.utf8";
const char* const USER_DICT_PATH = "/tmp/Tools/jieba_dict/dict/user.dict.utf8";
const char* const IDF_PATH = "/tmp/Tools/jieba_dict/dict/idf.utf8";
const char* const STOP_WORD_PATH = "/tmp/Tools/jieba_dict/dict/stop_words.utf8";



Index::Index() 
	: jieba(DICT_PATH,
        HMM_PATH,
        USER_DICT_PATH,
        IDF_PATH,
        STOP_WORD_PATH){} 


const DocInfo* Index::GetDocInfo(int64_t  doc_id){
	if(doc_id < 0 || doc_id >= forward_index.size() )
		return nullptr ;
	return &forward_index[doc_id] ;
}

const InvertedList* Index::GetInvertedList(const string& key){
	auto it = inverted_index.find(key) ;
	if( it == inverted_index.end()){
		return nullptr ;
	}
	return &it->second ;
}


// 核心操作：按照 \3 对 line 进行切分
// 第一个部分是标题
//   二        url
//   三	       content
DocInfo* Index::BuildForward(const string& line){
	//1、先把line按照 \3 切分成 3 个部分
	vector<string> tokens ;
	common::Util::Split(line,"\3",&tokens) ;
	if( tokens.size() != 3 ){
		// 如果切分结果不是 3 分
		// 就表示当前行失败
		return nullptr ;
	}	
	// 2、把切分对象填充到 DocInfo 对象中
	DocInfo doc_info ;
	doc_info.doc_id = forward_index.size() ;
	
	doc_info.title = tokens[0] ;
	doc_info.url = tokens[1] ;
	doc_info.content = tokens[2] ;

//	forward_index.push_back(doc_info) ;
//	vector 中默认存放内容，是对象的副本
//	要把 DocInfo 调用拷贝构造才能 push_back
//	涉及到对象的拷贝
//	doc_info 可能是一个比较大的对象（content 字段，html正文）
//	仔细观察可以发现，doc_info临时变量，在push_back之后就被释放
//	基于C++11的转移语义，直接把doc_info对象转移到vector中即可,就不需要拷贝构造
//	转移语义是基于OS的虚拟地址的影射来完成的
//	
//	vector 的 push_back 提供了 转移语义版本的.
//	要使用这个版本，需要让参数是一个右值引用.
//	使用 std::move 来进行类型转换，把普通的值转换成一个右值引用
	forward_index.push_back(std::move(doc_info)) ;
	// 3 返回结果
	// return &doc_info ; :返回的是野指针
	return &forward_index.back() ;
}


// 倒排是一个hash表
// key 是词（针对文档的分词结果）
// value 是倒排拉链（包含若干个 Weight 对象）
// 每次遍历到一个文档，就要分析这个文档，并且把相关信息，更新到倒排结构中.
void Index::BuildInverted(const DocInfo& doc_info){
	// 0、创建专门用于统计词频的结构
	struct WordCnt{
		int title_cnt ;
		int content_cnt ;

		WordCnt() : title_cnt(0) , content_cnt(0) {} 
	} ;
	unordered_map<string,WordCnt> word_cnt_map ;
	// 1、针对标题进行分词
	vector<string> title_token ;
	CutWord(doc_info.title,&title_token) ;
	// 2、遍历分词结果，统计每个分词出现的次数	
	// 	注意此处需要考虑大小写的问题
//	for(const auto& word : title_token){
//	 to_lower :原地修改 word 
	for(string word : title_token){
		// map/unorded_map [] 有两个功能，key不存在就添加，存在就修改
		// 在统计之前把单词统一转成小写的操作.
		boost::to_lower(word) ;
		++word_cnt_map[word].title_cnt ;
	}



	// 3、针对正文进行分词
	vector<string> content_token ;
	CutWord(doc_info.content,&content_token) ;
	// 4、遍历分词结果，统计每个词出现的次数
	for( string word : content_token ){
		boost::to_lower(word) ;
		++word_cnt_map[word].content_cnt ;
	}
	// 5、根据统计结果，整合出 Wieght 对象 ，并把结果更新到 倒排索引中即可
	// word_pair 每次循环就是对应到 map 中的一个键值对（std::pair)
	for(const auto& word_pair : word_cnt_map ){
		// 构造 Weight 对象
		Weight weight ;
		weight.doc_id = doc_info.doc_id ;
		// 权重 = 标题出现次数*10 + 正文出现次数
		weight.weight = 
			10*word_pair.second.title_cnt +  word_pair.second.content_cnt ;
		weight.word = word_pair.first ;
		
		// 把 weight 对象插入到 倒排索引中 ， 需要先找到对应的倒排拉链
		// 然后追加到拉链末尾即可
		InvertedList& inverted_list = inverted_index[word_pair.first] ;
		inverted_list.push_back(std::move(weight)) ;
	}
} 

void Index::CutWord(const string& input , vector<string>* output){
	jieba.CutForSearch(input,*output) ;
}

		
bool Index::Build(const string& input_path){
	// 1、按行读取输入文件内容
	// 上一个预处理文件模块生成的 raw_input 文件
	// 	raw_input 的结构：是一个行文本文件，每一刚对应一个文档
	// 	每一刚又分成三个部分，使用 \3 来切分，分别是标题，url，正文
	cout<<"开始构建索引"<<endl ;
	ifstream file(input_path.c_str()) ;
	if( !file.is_open()){
		cout<<"raw_nput 文件打开失败"<<endl ;
		return false ;
	}
	string line ;
	while( getline(file,line)){
	// 2、针对当前行，解析成 DocInfo 对象，并构造位正排索引
		DocInfo* doc_info = BuildForward(line) ;
		if( doc_info == nullptr){
			//当前文档构造正排出现错误
			cout<<"构建正排错误"<<endl ;
			continue ;
		}
	// 3、根据当前 DocInfo 对象，进行解析，构造倒排索引
		BuildInverted(*doc_info) ;
		// 打印 log , 调试程序
		if( doc_info->doc_id % 100 == 0 )
			cout<<doc_info->doc_id<<endl ;
	}
	cout<<"结束构建索引"<<endl ;
	file.close() ;
	return true ;
}

////////////////////////////////////////////////////////////////////
// 以下代码为 Searcher
////////////////////////////////////////////////////////////////////
bool Searcher::Init(const string& input_path){
	return index->Build(input_path) ;
}

// 这个函数进行的操作就是把查询词进行搜索，得到都所结果
bool Searcher::Search(const string& query , string* output){
	// 1、[分词] 针对查询词进行分词
	vector<string> tokens ;
	index->CutWord(query,&tokens) ;	
	// 2、[触发] 根据分词结果，查倒排，把相关的文档都获取到
	vector<Weight> all_token_result ;
	for(string word : tokens){
		// 做索引的时候，忽略了大小写
		boost::to_lower(word) ;
		auto* inverted_list = index->GetInvertedList(word) ;
		if( inverted_list == nullptr ){
			// 该词在倒排拉链中不存在
			// 如果这个词比较生僻，在所有的文档中都没出现过
			continue ;
		}	
		// tokens 包含多个结果，需要把国歌结果合并到一起，才能统一的排序
		all_token_result.insert( all_token_result.end() , inverted_list->begin() , inverted_list->end()) ;
	}
	// 3、[排序] 把刚才查到的这些文档的倒排拉链合并到一起并按照权重进行降序排列
	std::sort( all_token_result.begin() , all_token_result.end(),
	[](const Weight& w1 , const Weight& w2){
		// 实现降序排序
		return w1.weight > w2.weight ;
		// 实现升序排序
		// return w1.weight < w2.weight ;
	}) ;
	// 4、[包装结果] 把得到的这些倒排拉链中的 文档 id 获取到，然后去查正排，再把doc_info 中的内容构造成最终预期的格式.(JSON)
	// 使用 jsoncpp 来实现	
	// <jsoncpp/json/json.h>
	// Json::Value	
	Json::Value results ;
	for(const auto& weight : all_token_result ){
		// 根据 weight 中的 doc_id 查正排
		const DocInfo* doc_info = index->GetDocInfo(weight.doc_id) ;
		// 把这个 doc_info 对象再进一步的包装成一个 JSON 对象
		Json::Value result ;
		result["title"] = doc_info->title ;
		result["url"] = doc_info->url ;
		result["desc"] = GenerateDesc(doc_info->content,weight.word) ;
		results.append(result) ;
	}
	// 把得到的results这个 JSON 对象序列化成字符串，写入 output 中
	Json::FastWriter writer ;
	*output = writer.write(results) ;
	return true ;
}

string Searcher::GenerateDesc(const string& content ,const string& word){
	// 根据正文，找到 word 出现的位置
	// 以该词为中心，往前找 60 字节，作为描述的起始位置
	// 60 160
	// 再从其实位置往后找 160 字节，作为整个描述
	// 需要注意边界条件
	// 例如：前面不够60就从0开始，
	// 后面内容不够，就到末尾结束.
	// 后面内容现实不下，可以使用...省略来表示
	// [60/160] 拍脑门	
	// 1、先找到 word 在正文中出现的位置
	size_t first_pos = content.find(word) ;
	if( first_pos == string::npos){
		// 该词在正文总不存在，只在标题中存在等.
		// 如果找不到，就直接从头开始作为起始位置
		if( content.size() < 160){
			return content ;
		}
		string desc = content.substr(0,160) ;
		desc[desc.size()-1] = '.' ;
		desc[desc.size()-2] = '.' ;
		desc[desc.size()-3] = '.' ;
		return desc ;
	}
	// 2、找到了 first_pos 位置，以这个位置为基准，往前找一些字节
	size_t desc_beg = first_pos < 60 ? 0 : first_pos - 60 ;
	if( desc_beg + 160 >= content.size())
		return content.substr(desc_beg) ;
	else
	{
		string desc = content.substr(desc_beg,160) ;
		desc[desc.size()-1] = '.' ;
		desc[desc.size()-2] = '.' ;
		desc[desc.size()-3] = '.' ;
		return desc ;
	}
} 

} // namespace searcher	
