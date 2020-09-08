#include "searcher.h"

int main(){
	searcher::Searcher sh ;
	bool ret = sh.Init("../data/output/raw_input") ;
	if(!ret){
		cout<<"Searcher 初始化失败"<<endl ;
		return 1 ;
	}	
	while(true){
			// std::flush ==> fflush(stdout)
		cout<<"searcher > "<<std::flush ;
		string query ;
		std::cin>>query ;
		if(!std::cin.good()){
			//读到EOF
			cout<<"goodbye"<<endl ;
			break ;
		}	
		string results ;
		sh.Search(query,&results) ;
		cout<<results<<endl ;
	}
	return 0 ;
}
