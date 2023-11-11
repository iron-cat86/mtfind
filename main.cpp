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

template <typename T>
class ring_buffer
{
public:
    ring_buffer(size_t capacity):
        range(capacity+1),
        storage(capacity+1), 
        tail(0),
        head(0)
    {}
    
   bool push(T value, std::mutex &mt)
   { 
      mt.lock();
      size_t curr_tail=tail;
      size_t curr_head=head;

      if(get_next(curr_tail)==curr_head)
      {
         mt.unlock();
         return false;
      }
      storage[curr_tail]=std::move(value);
      tail=get_next(curr_tail);
      mt.unlock();
      return true;
   }

   bool pop(T &value, std::mutex &mt)
   {
      mt.lock();
      size_t curr_head=head;
      size_t curr_tail=tail;

      if(curr_head==curr_tail)
      {
         mt.unlock();
         return false;
      }
      value=std::move(storage[curr_head]);
      head=get_next(curr_head);
      mt.unlock();
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
    std::vector<T> storage;
    size_t         range=0;
    size_t         tail=0;
    size_t         head=0;
};

inline void insertAttachment(
   const std::string             &str, 
         size_t                   mask_length, 
         size_t                   m, 
         size_t                   step, 
         size_t                  &amountOfAtt, 
         size_t                  &lab, 
         size_t                   i,  
         std::mutex              &mt,
         std::vector<OutputData> &output
      )
{
   char att[mask_length];
   size_t start=m-step;
                  
   if(
      amountOfAtt==0||
      (amountOfAtt>0&&start>lab)
   )
   {
      for(size_t j=start; j<start+mask_length; ++j)
         att[j-start]=str[j];
      att[mask_length]=NULL;
      std::string attachment(att);
      OutputData data={i, start, attachment};
      mt.lock();
      output.push_back(data);
      mt.unlock();
      ++amountOfAtt;
      lab=start+mask_length-1;
   }
}

void findAttachments(
         std::vector<OutputData> &output, 
   const std::string             &mask, 
   const std::string             &str, 
         size_t                   i, 
         std::mutex              &mt
      )
{
   if(mask.length()>str.length())
      return;
   std::vector<std::vector<int64_t>> positions;
   size_t n=0;
   int64_t pos_num=-1;
   size_t amountOfAtt=0;
   size_t lab=0;
   
   for(size_t n=0; n<mask.length(); ++n)
   {
      if(n>0&&positions.size()<n)
         return;
      
      if(mask[n]=='?')
      {
         if(n==mask.length()-1)
         {
            if(pos_num<0)
            {
               std::cerr<<"Wrong search algorithm!\n";
               return;
            }
            
            for(size_t k=0; k<positions[pos_num].size(); ++k)
               insertAttachment(
                  str, 
                  mask.length(), 
                  positions[pos_num][k], 
                  pos_num, 
                  amountOfAtt, 
                  lab, 
                  i, 
                  mt, 
                  output
               );
         }
         else
            positions.push_back({-1});
      }
      else
      {
         size_t num=0;
         size_t step_num=pos_num>=0?n-pos_num:1;
         size_t m=(
                    pos_num>=0
                   ?positions[pos_num][0]+step_num
                   :n 
                  );
      
         while(
            (pos_num<0&&m<str.length()-mask.length()+1+n)||
            (pos_num>=0&&num<positions[pos_num].size())
         )
         {
            if(mask[n]==str[m])
            {
               if(n==mask.length()-1)
                  insertAttachment(
                     str, 
                     mask.length(), 
                     m, 
                     n, 
                     amountOfAtt, 
                     lab, 
                     i, 
                     mt, 
                     output
                  );
               else if(positions.size()==n)
                  positions.push_back({(int64_t)m});
               else
                  positions[n].push_back((int64_t)m);
            }
            
            if(pos_num<0)
               ++m;
            else
            {
               ++num;
               m=positions[pos_num][num]+step_num;
            }
         }
         pos_num=n;
      }
   }
}

void outputData(const std::vector<OutputData> &output)
{
   std::cout<<output.size()<<"\n";
   
   for(const OutputData &data: output)
      std::cout<<data.strNumber+1<<" "<<data.posNumber+1<<" "<<data.attachment<<"\n";
} 

std::vector<OutputData> getOutputData(const char *filename, const std::string &mask)
{  
   std::ifstream file(filename);
   std::vector<OutputData> output;
   
   if(!file.is_open())
   {
      std::cerr<<"It is impossible to read this file! Check file name, or path to file, or file existing.\n";
      return output;
   }
   else if(std::filesystem::file_size(filename)>1000000000)
   {   
      std::cerr<<"File is more than 1 GB!\n";
      return output;
   }
   int processor_count=std::thread::hardware_concurrency();
   
   if(processor_count<2)
   {
      std::clog<<"Warning : processor count for your computer is less 2! It will be 2 by default.\n";
      processor_count=2;
   }
   size_t file_str_count=0;
   size_t try_str_count=0;
   bool fileIsOver=false;
   ring_buffer<std::pair<std::string, size_t>> buffer(1024);
   std::mutex r_mt;
   std::mutex w_mt;
   std::mutex v_mt;
   std::thread read_thread=std::thread(
         [&](){
         std::string s;
            
         while(getline(file, s))
         {
            std::pair<std::string, size_t> inh=std::make_pair(s, file_str_count);
            
            while(!buffer.push(inh, r_mt));
               std::this_thread::yield();
            ++file_str_count;
         }
         fileIsOver=true;
      }
      );
   std::vector<std::thread> find_thread;
      
   for(int i=0; i<processor_count-1; ++i)
   {
      find_thread.emplace_back(std::thread(
         [&](){
         std::pair<std::string, size_t> inh;
            
         while(
            !(
               fileIsOver&&
               try_str_count>=file_str_count
            )
         )
         {
            bool took=false;
            
            while(
               !(
                  took||
                  (
                     (!took)&&
                     fileIsOver&&
                     try_str_count>=file_str_count
                  )
               )
            )
            {
               took=buffer.pop(inh, w_mt);
               std::this_thread::yield();
            }
            
            if(took)
            {
               findAttachments(output, mask, inh.first, inh.second, v_mt);
               w_mt.lock();
               ++try_str_count;
               w_mt.unlock();
            }
         }
      }
      ));
   } 
   read_thread.join();
      
   for(int i=0; i<processor_count-1; ++i)
      find_thread[i].join();
   file.close();
   std::sort(output.begin(), output.end(), OutputDataCompare);
   return output;
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
   std::vector<OutputData> controloutput=getOutputData(filename, mask);
   outputData(controloutput);
   
   for(int i=0; i<10000; ++i)
   {
      std::vector<OutputData> output=getOutputData(filename, mask);
      
      if(output.size()!=controloutput.size())
      {   
         std::cerr<<"Prog is not correct!\n";
         return 4;
      }
      
      for(size_t j=0; j<output.size(); ++j)
      {
         if(
            output[j].strNumber!=controloutput[j].strNumber||
            output[j].posNumber!=controloutput[j].posNumber||
            output[j].attachment!=controloutput[j].attachment
         )
         {
            std::cerr<<"Prog is not correct!\n";
            return 5;
         }
      }
   }
   return 0; 
}
