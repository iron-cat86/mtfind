/*Задача
Написать программу mtfind, производящую поиск подстроки в текстовом файле по маске с использованием
многопоточности.
Маска - это строка, где "?" обозначает любой символ.
Программа принимает в качестве параметров командной строки:
1. Имя текстового файла, в котором должен идти поиск (размер файла - до 1Гб).
2. Маску для поиска, в кавычках. Максимальная длина маски 1000 символов.
Вывод программы должен быть в следующем формате:
На первой строке - количество найденных вхождений.
Далее информация о каждом вхождении, каждое на отдельной строке, через пробел: номер строки, позиция в
строке, само найденное вхождение.
Порядок вывода найденных вхождений должен совпадать с их порядком в файле
Дополнения:
В текстовом файле кодировка только 7-bit ASCII
Поиск с учетом регистра
Каждое вхождение может быть только на одной строке. Маска не может содержать символа перевода строки
Найденные вхождения не должны пересекаться. Если в файле есть пересекающиеся вхождения то нужно
вывести одно из них (любое).
Пробелы и разделители участвуют в поиске наравне с другими символами.
Можно использовать STL, Boost, возможности С++1x.
Многопоточность нужно использовать обязательно. Однопоточные решения засчитываться не будут.
Серьезным плюсом будет разделение работы между потоками равномерно вне зависимости от количества строк
во входном файле.
Решение представить в виде архива с исходным кодом и проектом CMake или Visual Studio (либо в виде ссылки на
онлайн Git-репозиторий). Код должен компилироваться в GCC или MSVC.
ПРИМЕР
Файл input.txt:
I've paid my dues
Time after time.
I've done my sentence
But committed no crime.
And bad mistakes ?
I've made a few.
I've had my share of sand kicked in my face
But I've come through.
Запуск программы: mtfind input.txt "?ad"
Ожидаемый результат:
3
5 5 bad
6 6 mad
7 6 had*/
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <thread>

using namespace std;

struct OutputDate
{
   size_t strNumber=0;
   size_t posNumber=0;
   string attachment;
};

vector<string> read(char* filename)
{
   vector<string> answer; 
   ifstream file(filename);
   
   if(!file.is_open())
      cerr<<"It is impossible to read this file! Check file name, or path to file, or file existing.\n";
   else
   {
      string s;
   
      while(getline(file, s))
         answer.push_back(s);
      file.close();
   }
   return answer;
}

string getStr(string str, size_t start, size_t length)
{
   string answer="";
   
   if(start+length>str.length())
      cerr<<"Wrong string getting from \""<<str<<"\" with start="<<start<<" and length="<<length<<"!\n";
   else
   {
      for(size_t i=start; i<start+length; ++i)
         answer+=str.data()[i];
   }
   return answer;
}

bool strEqualMask(string mask, string str)
{
   if(mask.length()!=str.length())
      return false;
   
   size_t i=0;
   
   while(
      i<mask.length()&&
      (
         mask[i]==str[i]||
         mask[i]=='?'
      )
   )
      ++i;
   
   if(i<mask.length())
      return false;
   return true;
}

void find(vector<OutputDate> &output, const vector<string> &strings, const string &mask, size_t begin, size_t end)
{   
   if(!(begin>=end||end>strings.size()||end==0))
   {
      for(size_t i=begin; i<end; ++i)
      {
         string str=strings[i];
      
         if(str.length()>=mask.length())
         {
            size_t j=0;
         
            while(j<str.length()-mask.length())
            {
               string curr_str=getStr(str, j, mask.length());
               bool attIsFound=strEqualMask(mask, curr_str);
         
               while(
                  j<str.length()-mask.length()&&
                  (!attIsFound)
               )
               {
                  ++j;
                  curr_str=getStr(str, j, mask.length());
                  attIsFound=strEqualMask(mask, curr_str);
               }
         
               if(attIsFound)
               {   
                  output.push_back({i, j, curr_str});
                  j+=curr_str.length();
               }
            }
         }
      }
   }
   else
      cerr<<"Wrong begin and end parameters!\n";
}

void outInfo(const vector<OutputDate> &output)
{
   cout<<output.size()<<"\n";
   
   for(OutputDate date: output)
      cout<<date.strNumber<<" "<<date.posNumber<<" "<<date.attachment<<"\n";
} 

int main(int argc, char* argv[])
{
   if(argc!=3)
   {
      cerr<<"Wrong amount of arguments!\n";
      return 1;
   }
   char *filename=argv[1];
   string mask=argv[2];
   
   if(mask.length()>1000)
   {
      cerr<<"Mask is too long!\n";
      return 2;
   }
   vector<string> strings=read(filename);
   
   if(strings.empty())
   {
      cerr<<"Strings set is empty.\n";
      return 3;
   }
   int processor_count=thread::hardware_concurrency();
   
   if(processor_count<=0)
   {
      clog<<"Warning : processor count for your computer is not defind! It will be 1 by default.\n";
      processor_count=1;
   }
   size_t str_amount=strings.size()/processor_count;
   
   if(strings.size()%processor_count>0)
      ++str_amount;
   size_t start=0;
   vector<OutputDate> output;
   
   for(int i=0; i<processor_count; ++i)
   {
      size_t end=(i<processor_count-1)
                ?start+str_amount
                :strings.size();
      thread find_thread([&]() {find(output, strings, mask, start, end);});
      find_thread.join();
   }   
   outInfo(output);
   return 0;
}
