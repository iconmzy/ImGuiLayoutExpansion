#pragma once


#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#include "imgui.h"
#include "imgui_internal.h"
#include <vector>
#include <string>
#include <cassert>
#include <algorithm>


using CustomWindowFunc = void (*)();

namespace FrameGUILayout {

    class CustomLayoutNode;

    
    static inline float Clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }

    // A single node in the layout tree.
    // Node types:
    //  - Window node: holds a CustomWindowFunc; 
    //  - Splitter node: vertical (rows) or horizontal (columns). 
    class CustomLayoutNode {
    public:
        
        CustomLayoutNode(bool isVertical, const char* label = nullptr)
            : m_isVertical(isVertical), m_label(label ? label : (isVertical ? "Vertical" : "Horizontal"))
        {
            m_visible = true;
            m_splitterWidth = 1.0f;
            m_lastVisibleCount = 0;
            m_equalizeOnVisibleChange = true;
            m_minRatio = 0.05f; // min size per visible child windows 
            m_pfnCustomWindowFunc = nullptr;
            m_domainPos = ImVec2(0, 0);
            m_domainSize = ImVec2(0, 0);
        }

        
        CustomLayoutNode(CustomWindowFunc func, const char* label)
            : m_isVertical(false), m_label(label ? label : "Window"), m_pfnCustomWindowFunc(func)
        {
            m_visible = true;
            m_splitterWidth = 0.0f;
            m_lastVisibleCount = 0;
            m_equalizeOnVisibleChange = false;
            m_minRatio = 0.0f;
            m_domainPos = ImVec2(0, 0);
            m_domainSize = ImVec2(0, 0);
        }

        ~CustomLayoutNode() {
            for (auto* c : m_children) delete c;
        }

        
        void AddVerticalChild(CustomLayoutNode* child) {
            assert(IsVerticalSplitter() && "AddVerticalChild only valid on vertical splitter nodes");
            if (!child) return;
            m_children.push_back(child);
            m_splitRatios.resize(m_children.size(), 0.0f);
            m_lastVisibleCount = 0; // force equalize on next layout
        }
		void AddHorizontalChild(CustomLayoutNode* child) {
			assert(IsHorizontalSplitter() && "AddVerticalChild only valid on vertical splitter nodes");
			if (!child) return;
			m_children.push_back(child);
			m_splitRatios.resize(m_children.size(), 0.0f);
			m_lastVisibleCount = 0; // force equalize on next layout
		}

        
        void SetHorizontalChildren(CustomLayoutNode* left, CustomLayoutNode* middle = nullptr, CustomLayoutNode* right = nullptr) {
            assert(IsHorizontalSplitter() && "SetHorizontalChildren only valid on horizontal splitter nodes");
            m_children.clear();
            if (left)   m_children.push_back(left);
            if (middle) m_children.push_back(middle);
            if (right)  m_children.push_back(right);
            assert(m_children.size() <= 3 && "Horizontal nodes support max 3 children");
            m_splitRatios.resize(m_children.size(), 0.0f);
            m_lastVisibleCount = 0; // force equalize on next layout
        }

		void SetVerticalChildren(CustomLayoutNode* left, CustomLayoutNode* middle = nullptr, CustomLayoutNode* right = nullptr) {
			assert(IsVerticalSplitter() && "SetVerticalChildren only valid on Vertical splitter nodes");
			m_children.clear();
			if (left)   m_children.push_back(left);
			if (middle) m_children.push_back(middle);
			if (right)  m_children.push_back(right);
			assert(m_children.size() <= 3 && "Horizontal nodes support max 3 children");
			m_splitRatios.resize(m_children.size(), 0.0f);
			m_lastVisibleCount = 0; // force equalize on next layout
		}

        const std::string& GetLabel() const { return m_label; }
        void SetLabel(const char* label) { if (label) m_label = label; }

        
        void SetVisible(bool v) { m_visible = v; }
        bool IsVisibleFlag() const { return m_visible; }

        //DFS
        bool IsEffectivelyVisible() const {
            if (!m_visible) return false;
            if (IsWindowNode()) return true;
            for (auto* c : m_children) if (c && c->IsEffectivelyVisible()) return true;
            return false;
        }
		void SetDFSVisible(bool new_status){
            if (IsWindowNode()) return;
            SetVisible(new_status);
            for (auto* c : m_children) {
                c->SetVisible(new_status);
            }
			
		}


        
        void ResizeNodeAndChildren(ImVec2 newPos, ImVec2 newSize) {
            m_domainPos = newPos; m_domainSize = newSize;
            if (!IsEffectivelyVisible()) return; 

            if (IsWindowNode()) {
                
                ImGui::SetNextWindowPos(m_domainPos);
                ImGui::SetNextWindowSize(m_domainSize);
                if (m_pfnCustomWindowFunc) m_pfnCustomWindowFunc();
                return;
            }

            
            EqualizeIfVisibleCountChanged();

            
            std::vector<int> visIdx;
            visIdx.reserve(m_children.size());
            for (int i = 0; i < (int)m_children.size(); ++i)
                if (m_children[i] && m_children[i]->IsEffectivelyVisible()) visIdx.push_back(i);
            if (visIdx.empty()) return;

            
            float sum = 0.f;
            for (int i : visIdx) sum += m_splitRatios[i];
            if (sum <= 1e-6f) {
                float eq = 1.f / visIdx.size();
                for (int i : visIdx) m_splitRatios[i] = eq;
                sum = 1.f;
            }

            if (m_isVertical) {
                float accY = m_domainPos.y;
                const float totalH = m_domainSize.y;
                for (size_t k = 0; k < visIdx.size(); ++k) {
                    int i = visIdx[k];
                    float h = totalH * (m_splitRatios[i] / sum);
                    m_children[i]->ResizeNodeAndChildren(ImVec2(m_domainPos.x, accY), ImVec2(m_domainSize.x, h));
                    accY += h;
                }
            }
            else {
                float accX = m_domainPos.x;
                const float totalW = m_domainSize.x;
                for (size_t k = 0; k < visIdx.size(); ++k) {
                    int i = visIdx[k];
                    float w = totalW * (m_splitRatios[i] / sum);
                    m_children[i]->ResizeNodeAndChildren(ImVec2(accX, m_domainPos.y), ImVec2(w, m_domainSize.y));
                    accX += w;
                }
            }
        }

        // Rendering split lines and children 
        void RenderNodeAndChildren() {
            if (!IsEffectivelyVisible()) return;
            if (IsWindowNode()) return; 

            // Draw splitters between visible children
            std::vector<int> visIdx;
            for (int i = 0; i < (int)m_children.size(); ++i)
                if (m_children[i] && m_children[i]->IsEffectivelyVisible()) visIdx.push_back(i);
            if (visIdx.size() >= 2) {
                float sum = 0.f; for (int i : visIdx) sum += m_splitRatios[i]; if (sum <= 1e-6f) sum = 1.f;
                if (m_isVertical) {
                    float acc = m_domainPos.y;
                    for (size_t k = 0; k < visIdx.size() - 1; ++k) {
                        int i = visIdx[k];
                        acc += m_domainSize.y * (m_splitRatios[i] / sum);
                        ImGui::GetForegroundDrawList()->AddLine(
                            ImVec2(m_domainPos.x, acc),
                            ImVec2(m_domainPos.x + m_domainSize.x, acc),
                            IM_COL32(100, 100, 100, 255), m_splitterWidth);
                    }
                }
                else {
                    float acc = m_domainPos.x;
                    for (size_t k = 0; k < visIdx.size() - 1; ++k) {
                        int i = visIdx[k];
                        acc += m_domainSize.x * (m_splitRatios[i] / sum);
                        ImGui::GetForegroundDrawList()->AddLine(
                            ImVec2(acc, m_domainPos.y),
                            ImVec2(acc, m_domainPos.y + m_domainSize.y),
                            IM_COL32(100, 100, 100, 255), m_splitterWidth);
                    }
                }
            }

            
            for (auto* c : m_children) if (c) c->RenderNodeAndChildren();
        }

        
        bool FindHoveredSplitter(const ImVec2& mousePos, CustomLayoutNode*& outNode, int& outBoundaryIndex) {
            outNode = nullptr; outBoundaryIndex = -1;
            if (!IsEffectivelyVisible() || IsWindowNode()) {
                
            }
            else {
                const float pad = 8.f;
                std::vector<int> visIdx;
                for (int i = 0; i < (int)m_children.size(); ++i)
                    if (m_children[i] && m_children[i]->IsEffectivelyVisible()) visIdx.push_back(i);
                if (visIdx.size() >= 2) {
                    float sum = 0.f; for (int i : visIdx) sum += m_splitRatios[i]; if (sum <= 1e-6f) sum = 1.f;
                    if (m_isVertical) {
                        float acc = m_domainPos.y;
                        for (size_t k = 0; k < visIdx.size() - 1; ++k) {
                            int i = visIdx[k]; acc += m_domainSize.y * (m_splitRatios[i] / sum);
                            if (mousePos.y >= acc - pad && mousePos.y <= acc + pad &&
                                mousePos.x >= m_domainPos.x && mousePos.x <= m_domainPos.x + m_domainSize.x) {
                                outNode = this; outBoundaryIndex = (int)k; return true;
                            }
                        }
                    }
                    else {
                        float acc = m_domainPos.x;
                        for (size_t k = 0; k < visIdx.size() - 1; ++k) {
                            int i = visIdx[k]; acc += m_domainSize.x * (m_splitRatios[i] / sum);
                            if (mousePos.x >= acc - pad && mousePos.x <= acc + pad &&
                                mousePos.y >= m_domainPos.y && mousePos.y <= m_domainPos.y + m_domainSize.y) {
                                outNode = this; outBoundaryIndex = (int)k; return true;
                            }
                        }
                    }
                }
            }
            
            for (auto* c : m_children) if (c) {
                if (c->FindHoveredSplitter(mousePos, outNode, outBoundaryIndex)) return true;
            }
            return false;
        }

       
        bool HandleSplitterDragAt(int boundaryIndex, const ImVec2& mouseDelta) {
            if (boundaryIndex < 0) return false;
            if (!IsEffectivelyVisible() || IsWindowNode()) return false;

  
            std::vector<int> visIdx;
            for (int i = 0; i < (int)m_children.size(); ++i)
                if (m_children[i] && m_children[i]->IsEffectivelyVisible()) visIdx.push_back(i);
            if (visIdx.size() < 2) return false;
            if (boundaryIndex >= (int)visIdx.size() - 1) return false;

            const float total = m_isVertical ? m_domainSize.y : m_domainSize.x;
            if (total <= 1e-6f) return false;
            const float deltaRatio = (m_isVertical ? mouseDelta.y : mouseDelta.x) / total;
            if (deltaRatio == 0.f) return false;

           
            int iA = visIdx[boundaryIndex];
            int iB = visIdx[boundaryIndex + 1];

            // Compute current sum across visible to keep others unchanged
            float sumVis = 0.f; for (int i : visIdx) sumVis += m_splitRatios[i]; if (sumVis <= 1e-6f) sumVis = 1.f;

            float rA = m_splitRatios[iA] / sumVis;
            float rB = m_splitRatios[iB] / sumVis;

            float newA = Clamp01(rA + deltaRatio);
            float newB = Clamp01(rB - deltaRatio);
            const float minr = m_minRatio;
            newA = newA < minr ? minr : newA;
            newB = newB < minr ? minr : newB;

            float pairSum = rA + rB;
            if (newA + newB > pairSum) {
                float excess = (newA + newB) - pairSum;
                // Reduce whichever grew more
                if (deltaRatio > 0) newA -= excess; else newB -= excess;
                
                newA = (std::max)(minr, newA);
                newB = (std::max)(minr, newB);
            }

            float othersSum = sumVis - (m_splitRatios[iA] + m_splitRatios[iB]);
            float newPairNormSum = newA + newB;
            float scale = (sumVis - othersSum) / (newPairNormSum > 1e-6f ? newPairNormSum : 1.f);
            m_splitRatios[iA] = newA * scale;
            m_splitRatios[iB] = newB * scale;

            return true;
        }


        int VisibleChildCount() const {
            int c = 0; for (auto* x : m_children) if (x && x->IsEffectivelyVisible()) ++c; return c;
        }

        bool IsWindowNode() const { return m_pfnCustomWindowFunc != nullptr; }
        bool IsVerticalSplitter() const { return !IsWindowNode() && m_isVertical; }
        bool IsHorizontalSplitter() const { return !IsWindowNode() && !m_isVertical; }

        const std::vector<CustomLayoutNode*>& GetChildren() const { return m_children; }
        std::vector<CustomLayoutNode*>& GetChildren() { return m_children; }

        void EqualizeIfVisibleCountChanged() {
            if (IsWindowNode()) return;
            
            std::vector<int> visIdx; visIdx.reserve(m_children.size());
            for (int i = 0; i < (int)m_children.size(); ++i)
                if (m_children[i] && m_children[i]->IsEffectivelyVisible()) visIdx.push_back(i);
            size_t visCount = visIdx.size();
            if (!m_equalizeOnVisibleChange) { m_lastVisibleCount = visCount; return; }
            if (visCount == m_lastVisibleCount) return;
            m_lastVisibleCount = visCount;
            if (visCount == 0) return;
            float eq = 1.f / (float)visCount;
            for (size_t k = 0; k < visIdx.size(); ++k) m_splitRatios[visIdx[k]] = eq;
            
            for (int i = 0; i < (int)m_children.size(); ++i)
                if (std::find(visIdx.begin(), visIdx.end(), i) == visIdx.end()) m_splitRatios[i] = 0.f;
        }

       
        ImVec2 GetDomainPos() const { return m_domainPos; }
        ImVec2 GetDomainSize() const { return m_domainSize; }

    private:
        bool m_isVertical = false;
        std::string m_label;
        bool m_visible = true;

        ImVec2 m_domainPos{ 0,0 };
        ImVec2 m_domainSize{ 0,0 };

        std::vector<CustomLayoutNode*> m_children;
        std::vector<float> m_splitRatios; 

        float m_splitterWidth = 4.0f;
        float m_minRatio = 0.05f; // min visible size per child
        size_t m_lastVisibleCount = 0;
        bool m_equalizeOnVisibleChange = true;

        CustomWindowFunc m_pfnCustomWindowFunc = nullptr; 
    };

    class CustomLayout {
    public:
        explicit CustomLayout(CustomLayoutNode* root) : m_root(root) {
            assert(root != nullptr && "Root must not be null");
            m_activeBoundaryIndex = -1;
        }
        ~CustomLayout() { delete m_root; }

        void UpdateAndRender() {
            DrawControlPanel();

            // Resize tree to viewport work area
            ImGuiViewport* vp = ImGui::GetMainViewport();
            
            m_root->ResizeNodeAndChildren(vp->WorkPos, vp->WorkSize);
            m_lastViewportSize = vp->WorkSize;


            ImVec2 mousePos = ImGui::GetMousePos();
            if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                m_activeNode = nullptr; m_activeBoundaryIndex = -1;
                m_root->FindHoveredSplitter(mousePos, m_activeNode, m_activeBoundaryIndex);
            }


            if (m_activeNode) {
                ImGui::SetMouseCursor(m_activeNode->IsVerticalSplitter() ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeEW);
            }

     
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && m_activeNode) {
                ImVec2 delta = ImGui::GetMouseDragDelta();
                if (m_activeNode->HandleSplitterDragAt(m_activeBoundaryIndex, delta)) {
                    ImGui::ResetMouseDragDelta();
                    // Re-apply layout after drag
                    ImGuiViewport* vp2 = ImGui::GetMainViewport();
                    m_root->ResizeNodeAndChildren(vp2->WorkPos, vp2->WorkSize);
                }
            }

            // Render splitters recursively
            m_root->RenderNodeAndChildren();
        }

        CustomLayoutNode* GetRoot() { return m_root; }

    private:
        // always-on control panel
        void DrawControlPanel() {
            ImGui::SetNextWindowBgAlpha(0.9f);
            ImGui::Begin("Layout Control", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
            ImGuiWindow* win = ImGui::GetCurrentWindow();
            if (win)
                ImGui::BringWindowToDisplayFront(win);
            ImGui::Separator();

            auto& parentnodes = m_root->GetChildren();
            if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_FittingPolicyResizeDown | ImGuiTabBarFlags_Reorderable)) {
                for (int i = 0; i < (int)parentnodes.size(); ++i) {
                    CustomLayoutNode* parentnode = parentnodes[i]; if (!parentnode) continue;
                    std::string tabName = parentnode->GetLabel().empty() ? ("" + std::to_string(i)) : parentnode->GetLabel();
                    if (ImGui::BeginTabItem(tabName.c_str())) {
                        bool parentVis = parentnode->IsVisibleFlag();
                        if (ImGui::Checkbox("BOX Visible", &parentVis)) {
                            parentnode->SetDFSVisible(parentVis);

                        }
                        if (parentnode->IsVisibleFlag()) {
                            auto& childnodes = parentnode->GetChildren();
                            for (int c = 0; c < (int)childnodes.size(); ++c) {
                                CustomLayoutNode* childnode = childnodes[c]; if (!childnode) continue;
                                std::string label = childnode->GetLabel().empty() ? ("Child " + std::to_string(c)) : childnode->GetLabel();
                                bool v = childnode->IsVisibleFlag();
                                if (ImGui::Checkbox(label.c_str(), &v)) {
                                    childnode->SetVisible(v);
                                    //if every child not visible ,turn off parent
                                    parentnode->SetVisible(parentnode->IsEffectivelyVisible());
                                }
                            }
                             
// 							if (ImGui::Button("Show All Child data")) {
// 									for (auto* childnode : childnodes) if (childnode) childnode->SetVisible(true);
// 							}
// 							ImGui::SameLine();
// 							if (ImGui::Button("Hide All Child data")) {
// 									for (auto* childnode : childnodes) if (childnode) childnode->SetVisible(false);
// 									parentnode->SetVisible(false);
// 							}
                            
                        }

                        ImGui::EndTabItem();
                    }
                }
                ImGui::EndTabBar();
            }
            ImGui::End();
        }

        CustomLayoutNode* m_root = nullptr;
        ImVec2 m_lastViewportSize{ 0,0 };

        CustomLayoutNode* m_activeNode = nullptr;
        int m_activeBoundaryIndex = -1;
    };

} 
