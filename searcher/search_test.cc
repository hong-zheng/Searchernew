#include "searcher.h"


int main(){
	// 先创建一个 Index 实例
	searcher::Index index ;
	bool ret = index.Build("../data/output/raw_input") ;
	if( !ret ){
		cout<<"test_index 索引构建失败"<<endl ;
		return 1 ;
	}
	// 如果构建索引成功，就调用索引中的相关函数.（查正排+查倒排）
	auto* inverted_list = index.GetInvertedList("filesystem") ;
	for(const auto& weight : *inverted_list){

/*
		cerr<<"doc_id:"<<weight.doc_id
		<< "weight:"<<weight.weight<<endl ;
		auto* doc_info = index.GetDocInfo(weight.doc_id) ;
		cerr<<"title:"<<doc_info->title<<endl ;
		cerr<<"content:"<<doc_info->content<<endl ;
		cerr<<"=====================================================+"<<endl ;
*/
	}
	return  0 ;
}
