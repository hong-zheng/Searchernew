// 这个代码用于解析实现预处理模块
// 核心功能就是要读取并分析 boost 文档的 .html 内容
// 解析出每个文档的标题，url，正文（去除 html 标签）
// 最终将结果输出到一个行文本文件
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include "../common/util.hpp"

using namespace std ;


// 定义一些边关的变量和结构体

// 这个变量表示从哪一个目录中读取 boost 文档的 html
string g_input_path = "../data/input/" ;

// 这个变量对应预处理模块输出刚到哪里
string g_output_path = "../data/output/raw_input" ;

// 创建一个重要的结构体，表示一个文档（一个 HTML ）
struct DocInfo{
	// 文档的标题
	string title ;
	// 文档的 url 
	string url ;
	// 文档正文的内容
	string content ;	
} ;


// 输入参数：&
// 输出参数：*
// 输入输出参数:&
bool EnumFile(const string& input_path,vector<string>* file_list){
	// 枚举目录需要使用 boost 完成
	// 把 boost::filesystem 这个命名空间定义一个别名
	namespace fs = boost::filesystem ;
	fs::path root_path(input_path) ;
	if( !fs::exists(root_path)){
		cout<<"当前目录不存在"<<endl ;
		return false ;
	}
	// 递归遍历的时候使用用一个核心的类
	// 迭代器使用循环实现的时候可以自动完成递归
	// C++ 中的一种常见的做法，把迭代其的默认构造函数生成的迭代器作为一个“哨兵”
	fs::recursive_directory_iterator end_iter ;	
	for(fs::recursive_directory_iterator iter(root_path); iter != end_iter ;++iter){
		// 当前的路径对应的是不是一个普通文件.如果是目录，直接跳过.
		if( !fs::is_regular_file(*iter)){
			continue ;
		}
		// 当前路径对应的文件是不是一个 html 文件,如果是其他文件也跳过
		if(iter->path().extension() != ".html"){
			continue ;
		}
		// 把得到的路径加入到最终结果的 vector 中
		file_list->push_back( iter->path().string()) ;
	}	
	return true ;
}

// 找到 html 中的 title 标签
bool ParseTitle(const string& html,string& title){
	size_t beg = html.find("<title>") ;	
	if(beg == string::npos){
		cout<<"标题未找到"<<endl ;
		return false ;
	}
	size_t end = html.find("</title>") ;
	if( end == string::npos ){
		cout<<"end 标题的位置没找到"<<endl ;
		return false ;
	}
	beg += string("<title>").size() ;
	if( beg >= end){
		cout<<"标题位置不合法"<<endl ;
		return false ;
	}
	title = html.substr(beg,end-beg) ;
	return true ;
}

// 根据本地的路径获取在线文档的路径
// ../data/input/html/thread.html
// https://www.boost.org/doc/libs/1_53_0/doc/html/thread.html
// 把本地路径的后半部分截取出来，再拼装上在线路径的前缀就可以了
bool ParseUrl(const string& file_path,string* url){
	string url_head = "https://www.boost.org/doc/libs/1_53_0/doc/" ;
	string url_tail = file_path.substr(g_input_path.size() );
	*url = url_head + url_tail ;
	return true ;
}

// 根据读取出来的 html 文档去标签
bool ParseContent(const string& html,string* content){
	bool is_content = true ;
	for( auto c : html ){
		if( is_content){
			if(c == '<'){
				// 当前遇到了标签
				is_content = false ;
			} else {
				// 这里还要单独处理 \n , 预期的输出结果是一个行文本文件
				// 最终结果 raw_input 中每一行对应到一个原始的 html 文档
				// 此时就需要把 html 文档中原来的 \n 都干掉
				if( c=='\n'){
					c = ' ' ;
				}
				// 当前是普通字符，就把结果写入到 content 中
				content->push_back(c) ;
			}
		}else{
			// 说明当前不是正文
			//当前是标签
			if( c == '>'){
				is_content = true ;
			}
			// 标签里面的其他内容直接忽略掉
		}
	}
	return true ;
}

bool ParseFile(const string& file_path,DocInfo* doc_info){
	//1、先读取文件内容
	// 一次性将整个文件内容都读取出来
	string html ;
	// Read 这样的函数是一个比较底层的通用的函数，各个模块可能都会使用，因此放在common目录中
	bool ret = common::Util::Read(file_path,&html) ;
	if( !ret ){
		cout<<"解析文件失败！读取文件失败！"<<file_path<<endl ;
		return false ;
	}
	//2、根据文件内容解析出标题.(html 中有一个 title 标签）
	ret = ParseTitle(html,doc_info->title) ;	
	if( !ret ){
		cout<<"解析标题失败"<<endl ;
		return false ;
	}
	//3、根据文件的路径，构造出对应的在线文档的 url 
	ret = ParseUrl(file_path,&doc_info->url) ;
	if( !ret ){
		cout<<"解析 url 失败!"<<endl ;
		return false ;
	}
	//4、根据文档的内容，进行去标签，作为 doc_info 中的 content 字段内容
	ret = ParseContent(html,&doc_info->content) ;
	if( !ret ){
		cout<<"解析正文失败"<<endl ;
		return false ;
	}
	return true ;
}


// ofstream 类没有拷贝构造函数（不能拷贝）
// 按照参数传的时候，只能传引用或指针
// 此处还不能是 const 引用，否则无法执行里面的写文件操作
// 每个 doc_info 就是一行
void WriteOutput(const DocInfo& doc_info,ofstream& ofstream){
	ofstream<<doc_info.title<<"\3"<<doc_info.url<<"\3"<<doc_info.content<<endl ;	
}

// 预处理过程的核心流程
// 1、把 input 目录中所有有 html 路径都枚举出来
// 2、根据枚举出来的路径依次读取每个文件内容，并进行解析
// 3、把解析结果写入到最终的输出文件中
int main()
{
	// 1、进行枚举路径
	vector<string> file_list ;
	bool ret = EnumFile(g_input_path,&file_list) ;	
	if(!ret){
		cout<<"枚举路径失败!"<<endl ;
		return 1 ;
	}

	ofstream output_file(g_output_path.c_str()) ;
	if( !output_file.is_open()){
		cout<<"打开output文件失败"<<endl ;
		return 1 ;
	}
	// 2、遍历枚举出来的路径，针对每个文件，单独进行处理
	// C++11 已经是就标准了
	for(const auto& file_path : file_list){
		cout<<file_path<<endl ;
		// 先创建一个 DocInfo 对象
		DocInfo doc_info ;
		// 通过一个函数来负责这里的解析工作
		ret = ParseFile(file_path,&doc_info) ;
		if(!ret){
			cout<<"解析该文件失败"<<file_path<<endl ;
			continue ;
		}
		// 3、把解析出来的结果写到最终的输出文件中
		WriteOutput(doc_info,output_file) ;
	}
	output_file.close() ;
	return 0 ;
}




