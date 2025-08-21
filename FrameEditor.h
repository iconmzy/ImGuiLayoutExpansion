#include <vector>
#include <string>
#include <fstream>
#include <nlohmann/json.hpp> 
using json = nlohmann::json;


struct JsonTimelineItem {
    int id;
    std::string type;
    float start_time;
    float end_time;
    std::string content_preview;
    ImVec4 color;
};

struct TimelineState {
    float total_duration = 30.0f; 
    float visible_start = 0.0f;
    float visible_end = 10.0f;
    float zoom_level = 1.0f;
    float scroll_position = 0.0f;
    bool panning = false;
    ImVec2 pan_start;
    float pan_start_visible_start;
    float pan_start_visible_end;
    int selected_item = -1;
};

static std::vector<JsonTimelineItem> timeline_items;
static TimelineState timeline_state;

bool LoadTimelineFromJson(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    try {
        json data = json::parse(file);
        timeline_items.clear();
        
        if (data.contains("total_duration")) {
            timeline_state.total_duration = data["total_duration"];
        }
        
        if (data.contains("items") && data["items"].is_array()) {
            for (const auto& item : data["items"]) {
                JsonTimelineItem timeline_item;
                timeline_item.id = item.value("id", 0);
                timeline_item.type = item.value("type", "unknown");
                timeline_item.start_time = item.value("start_time", 0.0f);
                timeline_item.end_time = item.value("end_time", 0.0f);
                timeline_item.content_preview = item.value("content", "").substr(0, 20) + "...";    
                timeline_item.color = ImVec4(0.2f, 0.8f, 0.3f, 0.7f);
                
                timeline_items.push_back(timeline_item);
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}


static void JsonTimelineEditor() {
    ImGui::Begin("JSON Timeline Editor");
    
 
    if (ImGui::Button("Load JSON")) {

        LoadTimelineFromJson("data/timeline.json");
    }
    ImGui::SameLine();
    ImGui::Text("Zoom: %.1fx", timeline_state.zoom_level);
    ImGui::SameLine();
    if (ImGui::Button("Zoom In")) {
        timeline_state.zoom_level *= 1.2f;
        float visible_range = timeline_state.visible_end - timeline_state.visible_start;
        float center = (timeline_state.visible_start + timeline_state.visible_end) / 2.0f;
        timeline_state.visible_start = center - visible_range / (2.0f * 1.2f);
        timeline_state.visible_end = center + visible_range / (2.0f * 1.2f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Zoom Out")) {
        timeline_state.zoom_level /= 1.2f;
        float visible_range = timeline_state.visible_end - timeline_state.visible_start;
        float center = (timeline_state.visible_start + timeline_state.visible_end) / 2.0f;
        timeline_state.visible_start = center - visible_range * 1.2f / 2.0f;
        timeline_state.visible_end = center + visible_range * 1.2f / 2.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset View")) {
        timeline_state.visible_start = 0.0f;
        timeline_state.visible_end = 10.0f;
        timeline_state.zoom_level = 1.0f;
    }
    
    
    ImVec2 avail_size = ImGui::GetContentRegionAvail();
    float timeline_height = avail_size.y * 0.7f;
    float tracks_height = avail_size.y * 0.3f;
    
    
    ImGui::BeginChild("TimelineView", ImVec2(avail_size.x, timeline_height), true);
    
    
    if (ImPlot::BeginPlot("##TimeRuler", ImVec2(-1, 30), ImPlotFlags_NoChild | ImPlotFlags_CanvasOnly)) {
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_NoTickLabels);
        ImPlot::SetupAxisLimits(ImAxis_X1, timeline_state.visible_start, timeline_state.visible_end);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1);
        
        
        float time_range = timeline_state.visible_end - timeline_state.visible_start;
        float major_interval = std::pow(10.0f, std::floor(std::log10(time_range)));
        float minor_interval = major_interval / 5.0f;
        
        
        float first_major = std::floor(timeline_state.visible_start / major_interval) * major_interval;
        
        
        for (float t = first_major; t <= timeline_state.visible_end; t += major_interval) {
            if (t >= timeline_state.visible_start) {
                ImPlot::PlotLine("##MajorTick", &t, &t, 1, 0, 0, sizeof(float));
                char label[32];
                snprintf(label, sizeof(label), "%.1f", t);
                ImPlot::PlotText(label, t, 0.5f);
            }
        }
        
        ImPlot::EndPlot();
    }
    
    
    if (ImPlot::BeginPlot("##Timeline", ImVec2(-1, -1), ImPlotFlags_NoChild | ImPlotFlags_CanvasOnly)) {
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_NoTickLabels);
        ImPlot::SetupAxisLimits(ImAxis_X1, timeline_state.visible_start, timeline_state.visible_end);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1);
        
        
        if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            if (!timeline_state.panning) {
                timeline_state.panning = true;
                timeline_state.pan_start = ImGui::GetMousePos();
                timeline_state.pan_start_visible_start = timeline_state.visible_start;
                timeline_state.pan_start_visible_end = timeline_state.visible_end;
            } else {
                ImVec2 mouse_delta = ImGui::GetMousePos() - timeline_state.pan_start;
                float time_per_pixel = (timeline_state.pan_start_visible_end - timeline_state.pan_start_visible_start) / ImGui::GetWindowWidth();
                float time_delta = mouse_delta.x * time_per_pixel;
                
                timeline_state.visible_start = timeline_state.pan_start_visible_start - time_delta;
                timeline_state.visible_end = timeline_state.pan_start_visible_end - time_delta;
                
                
                timeline_state.visible_start = std::max(0.0f, std::min(timeline_state.visible_start, timeline_state.total_duration - (timeline_state.visible_end - timeline_state.visible_start)));
                timeline_state.visible_end = timeline_state.visible_start + (timeline_state.pan_start_visible_end - timeline_state.pan_start_visible_start);
            }
        } else {
            timeline_state.panning = false;
        }
        
      
        if (ImGui::IsWindowHovered()) {
            float mouse_wheel = ImGui::GetIO().MouseWheel;
            if (mouse_wheel != 0.0f) {
                float mouse_x_time = ImPlot::GetPlotMousePos().x;
                float zoom_factor = (mouse_wheel > 0) ? 1.1f : 1.0f / 1.1f;
                
                float new_visible_start = mouse_x_time - (mouse_x_time - timeline_state.visible_start) * zoom_factor;
                float new_visible_end = mouse_x_time + (timeline_state.visible_end - mouse_x_time) * zoom_factor;
                
                
                if ((new_visible_end - new_visible_start) > 0.1f && 
                    (new_visible_end - new_visible_start) < timeline_state.total_duration) {
                    timeline_state.visible_start = new_visible_start;
                    timeline_state.visible_end = new_visible_end;
                    timeline_state.zoom_level *= (mouse_wheel > 0) ? 1.1f : 1.0f / 1.1f;
                }
            }
        }
        
        
        for (const auto& item : timeline_items) {
            if (item.end_time < timeline_state.visible_start || item.start_time > timeline_state.visible_end) {
                continue;
            }
            
            float start_x = std::max(item.start_time, timeline_state.visible_start);
            float end_x = std::min(item.end_time, timeline_state.visible_end);
            
          
            float normalized_start = (start_x - timeline_state.visible_start) / (timeline_state.visible_end - timeline_state.visible_start);
            float normalized_end = (end_x - timeline_state.visible_start) / (timeline_state.visible_end - timeline_state.visible_start);
            float normalized_width = normalized_end - normalized_start;
            
            
            ImVec2 rect_min(ImGui::GetWindowPos().x + normalized_start * ImGui::GetWindowWidth(), 
                           ImGui::GetWindowPos().y + ImGui::GetWindowHeight() * 0.3f);
            ImVec2 rect_max(ImGui::GetWindowPos().x + normalized_end * ImGui::GetWindowWidth(), 
                           ImGui::GetWindowPos().y + ImGui::GetWindowHeight() * 0.7f);
            
            ImGui::GetWindowDrawList()->AddRectFilled(rect_min, rect_max, ImColor(item.color));
            ImGui::GetWindowDrawList()->AddRect(rect_min, rect_max, IM_COL32(255, 255, 255, 255));
            
            
            if (normalized_width > 0.1f) { 
                ImGui::GetWindowDrawList()->AddText(
                    ImVec2(rect_min.x + 5, rect_min.y + 5), 
                    IM_COL32(255, 255, 255, 255), 
                    item.content_preview.c_str()
                );
            }
            
            
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && 
                ImGui::IsMouseHoveringRect(rect_min, rect_max)) {
                timeline_state.selected_item = item.id;
            }
        }
        
        
        float current_time = (timeline_state.visible_start + timeline_state.visible_end) / 2.0f; 
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(ImGui::GetWindowPos().x + ((current_time - timeline_state.visible_start) / (timeline_state.visible_end - timeline_state.visible_start)) * ImGui::GetWindowWidth(), 
                  ImGui::GetWindowPos().y),
            ImVec2(ImGui::GetWindowPos().x + ((current_time - timeline_state.visible_start) / (timeline_state.visible_end - timeline_state.visible_start)) * ImGui::GetWindowWidth(), 
                  ImGui::GetWindowPos().y + ImGui::GetWindowHeight()),
            IM_COL32(255, 0, 0, 255), 2.0f
        );
        
        ImPlot::EndPlot();
    }
    
    ImGui::EndChild();
    
    
    ImGui::BeginChild("TracksView", ImVec2(avail_size.x, tracks_height), true);
    ImGui::Text("Track Details");
    ImGui::Separator();
    
    if (timeline_state.selected_item >= 0) {
        
        for (const auto& item : timeline_items) {
            if (item.id == timeline_state.selected_item) {
                ImGui::Text("ID: %d", item.id);
                ImGui::Text("Type: %s", item.type.c_str());
                ImGui::Text("Start: %.2f", item.start_time);
                ImGui::Text("End: %.2f", item.end_time);
                ImGui::Text("Duration: %.2f", item.end_time - item.start_time);
                break;
            }
        }
    } else {
        ImGui::Text("No item selected");
    }
    
    ImGui::EndChild();
    
    ImGui::End();
}