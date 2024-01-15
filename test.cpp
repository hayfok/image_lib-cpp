#include <iostream>
#include <vector>

int main()
{
   std::vector<int>* sig_buff = new std::vector<int>;
  

    for(int i { }; i < 3; ++i)
    {
        sig_buff->push_back(i);
        std::cout << sig_buff->at(i) << "\n";
    };

   
  
}
