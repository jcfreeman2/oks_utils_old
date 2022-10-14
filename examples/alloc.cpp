#include <sys/time.h>
#include <sys/resource.h>

#include <chrono>
#include <list>
#include <iostream>

#include <boost/pool/pool_alloc.hpp>


  // Normal structure

class Test {
  int i;
  void * p;
};


  // Use Boost pool (with mutexes)

class Test3 : public Test {

public:
  void * operator new(size_t) {return boost::fast_pool_allocator<Test3>::allocate();}
  void operator delete(void *ptr) {boost::fast_pool_allocator<Test3>::deallocate(reinterpret_cast<Test3*>(ptr));}

};


  // Use Boost pool (without mutexes)

class Test4 : public Test {

public:
  void * operator new(size_t) {return boost::fast_pool_allocator<Test4, boost::default_user_allocator_new_delete, boost::details::pool::null_mutex>::allocate();}
  void   operator delete(void *ptr) {boost::fast_pool_allocator<Test4, boost::default_user_allocator_new_delete, boost::details::pool::null_mutex>::deallocate(reinterpret_cast<Test4*>(ptr));}
};




const size_t ArraySize  =  20000000;
const size_t ListSize   =  20000000;


int main()
{
  size_t count;

  Test  ** t_array  = new Test  * [ArraySize];
  Test3	** t3_array = new Test3 * [ArraySize];
  Test4	** t4_array = new Test4 * [ArraySize];

  for(count=0; count < ArraySize; count++) {
    t_array[count] = 0;
    t3_array[count] = 0;
    t4_array[count] = 0;
  }

  std::cout << "Create and delete " << ArraySize << " objects (" << sizeof(Test) << " bytes per object)\n";

////////////////////////////////////////////////////////////////////////////////

  auto tp = std::chrono::steady_clock::now();

  for(count=0; count < ArraySize; count++)
    t_array[count] = new Test();

  for(count=0; count < ArraySize; count++)
    delete t_array[count];

  std::cout << " * standard operators new and delete require " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-tp).count() / 1000. << " seconds" << std::endl;


////////////////////////////////////////////////////////////////////////////////

  tp = std::chrono::steady_clock::now();

  for(count=0; count < ArraySize; count++)
    t4_array[count] = new Test4();

  for(count=0; count < ArraySize; count++)
    delete t4_array[count];

  std::cout << " * Boost operators new and delete require " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-tp).count() / 1000. << " seconds (no mutexes)" << std::endl;


////////////////////////////////////////////////////////////////////////////////

  tp = std::chrono::steady_clock::now();

  for(count=0; count < ArraySize; count++)
    t3_array[count] = new Test3();

  for(count=0; count < ArraySize; count++)
    delete t3_array[count];

  std::cout << " * Boost operators new and delete require " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-tp).count() / 1000. << " seconds (with mutexes)" << std::endl;

////////////////////////////////////////////////////////////////////////////////

  delete [] t_array;
  delete [] t3_array;
  delete [] t4_array;

////////////////////////////////////////////////////////////////////////////////

  std::cout << "Create and free list with " << ListSize << " integers\n";

  std::list<size_t> l;

  tp = std::chrono::steady_clock::now();

  for(count=0; count < ListSize; count++)
    l.push_back(count);

  while(!l.empty()) l.pop_front();

  std::cout << " * operation with list's standard allocator requires " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-tp).count() / 1000. << " seconds" << std::endl;

////////////////////////////////////////////////////////////////////////////////

  std::list<size_t, boost::fast_pool_allocator<size_t, boost::default_user_allocator_new_delete, boost::details::pool::null_mutex> > l4;

  tp = std::chrono::steady_clock::now();

  for(count=0; count < ListSize; count++)
    l4.push_back(count);

  while(!l4.empty()) l4.pop_front();

  std::cout << " * operation with list's Boost allocator requires " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-tp).count() / 1000. << " seconds (no mutexes)" << std::endl;

////////////////////////////////////////////////////////////////////////////////

  std::list<size_t, boost::fast_pool_allocator<size_t> > l3;

  tp = std::chrono::steady_clock::now();

  for(count=0; count < ListSize; count++)
    l3.push_back(count);

  while(!l3.empty()) l3.pop_front();

  std::cout << " * operation with list's Boost allocator requires " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-tp).count() / 1000. << " seconds (with mutexes)" << std::endl;

////////////////////////////////////////////////////////////////////////////////

  return 0;
}
