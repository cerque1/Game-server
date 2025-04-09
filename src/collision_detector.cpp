#include "collision_detector.h"
#include <cassert>

namespace collision_detector {

CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c) {
    // Проверим, что перемещение ненулевое.
    // Тут приходится использовать строгое равенство, а не приближённое,
    // пскольку при сборе заказов придётся учитывать перемещение даже на небольшое
    // расстояние.
    assert(b.x != a.x || b.y != a.y);
    const double u_x = c.x - a.x;
    const double u_y = c.y - a.y;
    const double v_x = b.x - a.x;
    const double v_y = b.y - a.y;
    const double u_dot_v = u_x * v_x + u_y * v_y;
    const double u_len2 = u_x * u_x + u_y * u_y;
    const double v_len2 = v_x * v_x + v_y * v_y;
    const double proj_ratio = u_dot_v / v_len2;
    const double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;

    return CollectionResult(sq_distance, proj_ratio);
}

std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider) {
    std::vector<GatheringEvent> events;
    for(size_t item_index = 0; item_index < provider.ItemsCount(); ++item_index){
        for(size_t gatherer_index = 0; gatherer_index < provider.GatherersCount(); ++gatherer_index){
            auto gatherer_info = provider.GetGatherer(gatherer_index);
            if(gatherer_info.start_pos == gatherer_info.end_pos){
                continue;
            }
            CollectionResult try_collect_point_res = TryCollectPoint(gatherer_info.start_pos, gatherer_info.end_pos, provider.GetItem(item_index).position);

            if(try_collect_point_res.IsCollected(gatherer_info.width)){
                events.push_back(GatheringEvent{item_index, gatherer_index, try_collect_point_res.sq_distance, try_collect_point_res.proj_ratio});
            }
        }
    }

    std::sort(events.begin(), events.end(), [](const GatheringEvent& lhs, const GatheringEvent& rhs){      
        return lhs.time < rhs.time;
    });
    return events;
}


}  // namespace collision_detector