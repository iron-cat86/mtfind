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

struct OutputData
{
   size_t      strNumber=0;
   size_t      posNumber=0;
   std::string attachment;
};

struct
{
   bool operator()(const OutputData &a, const OutputData &b) const 
   {
      return (
         (a.strNumber<b.strNumber)||
         (a.strNumber==b.strNumber&&a.posNumber<b.posNumber)
      );
   }
}
OutputDataCompare;

class ring_string_buffer
{
public:
    ring_string_buffer(size_t capacity):
        range(capacity + 1)
    {
       storage=new std::pair<std::string, size_t>[range];
    }
    
    inline bool push(const std::string &value, size_t strNum, std::mutex &m_mutex)
    { 
        std::unique_lock<std::mutex> t_locker(m_mutex);
        size_t gn=get_next(tail);
              
        if(gn==head)
           return false;
        storage[tail]=std::move(std::make_pair(value, strNum));
        tail=gn;
        return true;
    }

    inline bool pop(std::string &value, size_t &strNum, std::mutex &m_mutex)
    {
        std::unique_lock<std::mutex> t_locker(m_mutex);
        
        if(head==tail)
           return false;
        value=std::move(storage[head].first);
        strNum=std::move(storage[head].second);
        head=get_next(head);
        return true;
    }

private:
    inline size_t get_next(size_t slot) const
    {
       if(slot>=range)
          return 0;
       return slot+1;
    }

private:
    std::pair<std::string, size_t>* storage;
    size_t                          range=0;
    size_t                          tail=0;
    size_t                          head=0;
};

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

bool readLine(std::string &s, std::ifstream &file, size_t &curr_str_count, size_t &string_count, std::mutex &mt)
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

void find(std::vector<OutputData> &output, const std::string &mask, const std::string &str, const size_t &i, std::mutex &mt)
{      
   if(str.length()>=mask.length())
   {
      size_t j=0;
         
      while(j<str.length()-mask.length())
      {
         std::string curr_str=str.substr(j, mask.length());
         bool attIsFound=strEqualMask(mask, curr_str);
         
         while(
            j<str.length()-mask.length()&&
            (!attIsFound)
         )
         {
            ++j;
            curr_str=str.substr(j, mask.length());
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

void sortOutAndCheckInfo(std::vector<OutputData> &output, std::vector<OutputData> &firstData, int n)
{
   std::sort(output.begin(), output.end(), OutputDataCompare);
   
   if(n==0)
      std::cout<<output.size()<<"\n";
   size_t t=0;
   
   for(const OutputData &data: output)
   {
      if(n==0)
      {
         firstData.push_back(data);
         std::cout<<data.strNumber+1<<" "<<data.posNumber+1<<" "<<data.attachment<<"\n";
      }
      else
      {
         if(
            data.strNumber!=firstData[t].strNumber||
            data.posNumber!=firstData[t].posNumber||
            data.attachment!=firstData[t].attachment
         )
            std::clog<<"Program is not correct!\n";
         ++t;
      }
   }
} 

int mainRun(const char *filename, const std::string &mask, size_t amount_of_try)
{
   std::vector<OutputData> firstData;
   
   for(int n=0; n<amount_of_try; ++n)
   {
      std::mutex mt;
      size_t string_count=0;
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
      std::vector<OutputData> output;
      ring_string_buffer buffer(1024);
      std::thread read_thread=std::thread(
            [&](){
            size_t curr_str_count=0;
            std::string s="";
            bool lineIsRead=readLine(s, file, curr_str_count, string_count, mt);
            
            while(lineIsRead)   
            {
               buffer.push(s, curr_str_count, mt);
               lineIsRead=readLine(s, file, curr_str_count, string_count, mt);
            }
         }
         );
      std::vector<std::thread> find_thread;
      
      for(int i=0; i<processor_count; ++i)
      {
         find_thread.emplace_back(std::thread(
            [&](){
            std::string s;
            size_t curr_str_count=0;
            bool lineIsAdded=buffer.pop(s, curr_str_count, mt);
            
            while(lineIsAdded)   
            {
               find(output, mask, s, curr_str_count, mt);
               lineIsAdded=buffer.pop(s, curr_str_count, mt);
            }
         }
         ));
      } 
      read_thread.join();
      
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
