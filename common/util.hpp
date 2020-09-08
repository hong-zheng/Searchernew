#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <boost/algorithm/string.hpp>

using namespace std ;
using namespace boost ;

namespace common{

	class Util{
	public:
		// 负责从指定的路径中，读取出文件的整体内容，读到 output 这个 string 中
		static bool Read(const string& input_path,string* output){
			fstream file(input_path.c_str()) ;
			if( !file.is_open()){
				return false ;
			}			
			// 读取整个文件内容，思路监督，只要按行读取就行了，把读到的每行结果追加到 output 中即可
			// getline 功能就是读取文件中的一行
			// 如果读取成功，就把内容放到 line 中，并返回 true
			// 如果读取失败（读到文件末尾）, return false 
			string line ;
			while( getline(file,line)){	
				*output += ( line + "\n") ;
			}
			file.close() ;
			return true ;
		}
	
		// 基于 boost 中的字符串切分，封装一下
		static void Split(const string& input,const string& delimiter,vector<string>* output){
		// is_any_of(delimiter) : 表示 delimiter 中任意一个字符 都可以表示 分割符
		// token_compress_off : 表示关闭压缩,不会关闭压缩结果
		// aaaaa\3\3ddd => aaaaa "" ddd
		boost::split(*output,input,is_any_of(delimiter),token_compress_off) ;
	}
	} ;

} // namespace common
