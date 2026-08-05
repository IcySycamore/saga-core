#define COMPONENT_GETTER

#ifdef COMPONENT_GETTER

#ifndef COMPONENT_REGISTER
#define COMPONENT_REGISTER(__Index__, __Label__, __RT__)                       \
  inline __RT__ *get##__Label__(EntityInstance *item) {                        \
    return item->getComponent<__RT__>(__Index__);                              \
  }
#endif
#endif
/*
 * 若要使用此宏，请将一个项目中不同类型的EntityInstance在持久化文件（.json）中使用的索引对齐，
 * 也即一个数字代表一个语义，如小血瓶中      "30":"5" //当前消耗次数
 * 则药丸中也应"30":"1" //当前消耗次数 而非 "29":"1" //当前消耗次数
 * usage:
 *    COMPONENT_REGISTER((30),(ContainerCounter),(CounterComponent))
 * provided:
 *    CounterComponent *getContainerCounter()(EntityInstance
 * *item){//defination...}
 */