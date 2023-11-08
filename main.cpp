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
#include <algorithm>
#include <filesystem>
#include <mutex>
#include <atomic>

struct OutputDate
{
   size_t      strNumber=0;
   size_t      posNumber=0;
   std::string attachment;
};

struct
{
   bool operator()(OutputDate a, OutputDate b) const 
   {
      return (
         (a.strNumber<b.strNumber)||
         (a.strNumber==b.strNumber&&a.posNumber<b.posNumber)
      );
   }
}
OutputDateCompare;

std::mutex mt;
size_t string_count=0;

std::string getStr(const std::string &str, size_t start, size_t length)
{
   std::string answer="";
   
   if(start+length>str.length())
      std::cerr<<"Wrong string getting from \""<<str<<"\" with start="<<start<<" and length="<<length<<"!\n";
   else
   {
      for(size_t i=start; i<start+length; ++i)
         answer+=str.data()[i];
   }
   return answer;
}

bool strEqualMask(const std::string &mask, const std::string &str)
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

bool readLine(std::string &s, std::ifstream &file, size_t &curr_str_count)
{
   mt.lock();
   size_t curfilepos=file.tellg();   
   getline(file, s);
   
   if(file.tellg()==curfilepos)
   {
      mt.unlock();
      return false;
   }
   curr_str_count=string_count;
   ++string_count;
   mt.unlock();
   return true;
}

void find(std::vector<OutputDate> &output, const std::string &mask, const std::string &str, const size_t &i)
{      
   if(str.length()>=mask.length())
   {
      size_t j=0;
         
      while(j<str.length()-mask.length())
      {
         std::string curr_str=getStr(str, j, mask.length());
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
            mt.lock();
            output.push_back({i, j, curr_str});
            mt.unlock();
            j+=curr_str.length();
         }
      }
   }
}

void sortOutAndCheckInfo(std::vector<OutputDate> &output, std::vector<OutputDate> &firstData, int n)
{
   std::sort(output.begin(), output.end(), OutputDateCompare);
   
   if(n==0)
      std::cout<<output.size()<<"\n";
   size_t t=0;
   
   for(OutputDate date: output)
   {
      if(n==0)
      {
         firstData.push_back(date);
         std::cout<<date.strNumber+1<<" "<<date.posNumber+1<<" "<<date.attachment<<"\n";
      }
      else
      {
         if(
            date.strNumber!=firstData[t].strNumber||
            date.posNumber!=firstData[t].posNumber||
            date.attachment!=firstData[t].attachment
         )
            std::clog<<"Program is not correct!\n";
         ++t;
      }
   }
} 

int mainRun(const char *filename, const std::string &mask, size_t amount_of_try)
{
   std::vector<OutputDate> firstData;
   
   for(int n=0; n<amount_of_try; ++n)
   {
      string_count=0;
      std::ifstream file(filename);
   
      if(!file.is_open())
      {
         std::cerr<<"It is impossible to read this file! Check file name, or path to file, or file existing.\n";
         return 4;
      }
      else if(std::filesystem::file_size(filename)>1000000000)
      {   
         std::cerr<<"File is more than 1 GB!\n";
         return 5;
      }
      int processor_count=std::thread::hardware_concurrency();
   
      if(processor_count<=0)
      {
         std::clog<<"Warning : processor count for your computer is not defined! It will be 1 by default.\n";
         processor_count=1;
      }
      std::vector<OutputDate> output;
      std::vector<std::thread> find_thread;
   
      for(int i=0; i<processor_count; ++i)
      {
         find_thread.push_back(std::thread(
            [&](){
            size_t curr_str_count=0;
            std::string s="";
            bool reading=readLine(s, file, curr_str_count);
            
            while(reading)   
            {
               find(output, mask, s, curr_str_count);
               reading=readLine(s, file, curr_str_count);
            }
         }
         ));
      } 
   
      for(int i=0; i<processor_count; ++i)
         find_thread[i].join();
      file.close();
      sortOutAndCheckInfo(output, firstData, n);
   }
   return 0;
}

int main(int argc, char* argv[])
{
   if(argc!=3)
   {
      std::cerr<<"Wrong amount of arguments! "<<argc<<"\nUsage: mtfind <input file> \"<mask>\"\n";
      return 1;
   }
   const char *filename=argv[1];
   const std::string mask=argv[2];
   
   if(mask.length()>1000)
   {
      std::cerr<<"Mask is too long! "<<mask.length()<<"\n Use mask length less 1000 symbols.\n";
      return 2;
   }
      
   if(mask.contains("\n"))
   {
      std::cerr<<"Mask can not contain \"\\n\"-symbol!\n";
      return 3;
   }   
   return mainRun(filename, mask, 1000);
}
