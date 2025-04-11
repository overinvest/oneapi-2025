#include "integral_oneapi.h"

float IntegralONEAPI(float start, float end, int count, sycl::device device) {
    const float step = (end - start) / count;
    const float area = step * step;
    float result = 0.0f;

    try {
        sycl::queue q(device);
        
        // Создаем буфер для результата
        sycl::buffer<float, 1> result_buf(&result, 1);
        
        // Вычисляем интеграл
        q.submit([&](sycl::handler& h) {
            auto result_acc = result_buf.get_access<sycl::access::mode::write>(h);
            
            // Используем nd_range для лучшего контроля над работой групп
            h.parallel_for(sycl::nd_range<2>(
                sycl::range<2>(count, count),
                sycl::range<2>(16, 16)  // Размер рабочей группы
            ), [=](sycl::nd_item<2> item) {
                const int i = item.get_global_id(0);
                const int j = item.get_global_id(1);
                
                // Вычисляем координаты центра прямоугольника
                const float x = start + (i + 0.5f) * step;
                const float y = start + (j + 0.5f) * step;
                
                // Атомарно добавляем результат
                sycl::atomic_ref<float, sycl::memory_order::relaxed,
                               sycl::memory_scope::device,
                               sycl::access::address_space::global_space>
                    result_ref(result_acc[0]);
                result_ref.fetch_add(sycl::sin(x) * sycl::cos(y) * area);
            });
        }).wait();
        
        // Получаем результат
        auto result_acc = result_buf.get_access<sycl::access::mode::read>();
        result = result_acc[0];
        
    } catch (const sycl::exception& e) {
        std::cerr << "SYCL Exception: " << e.what() << std::endl;
        throw;
    }
    
    return result;
}