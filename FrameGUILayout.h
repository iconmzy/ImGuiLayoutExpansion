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
#include "imgui.h"
#include "implot.h"

using CustomWindowFunc = void (*)();

namespace FrameGUILayout {

	// utility structure for realtime plot
struct ScrollingBuffer {
    int MaxSize;
    int Offset;
    ImVector<ImVec2> Data;
    ScrollingBuffer(int max_size = 2000) {
        MaxSize = max_size;
        Offset  = 0;
        Data.reserve(MaxSize);
    }
    void AddPoint(float x, float y) {
        if (Data.size() < MaxSize)
            Data.push_back(ImVec2(x,y));
        else {
            Data[Offset] = ImVec2(x,y);
            Offset =  (Offset + 1) % MaxSize;
        }
    }
    void Erase() {
        if (Data.size() > 0) {
            Data.shrink(0);
            Offset  = 0;
        }
    }
};

// utility structure for realtime plot
struct RollingBuffer {
    float Span;
    ImVector<ImVec2> Data;
    RollingBuffer() {
        Span = 10.0f;
        Data.reserve(2000);
    }
    void AddPoint(float x, float y) {
        float xmod = fmodf(x, Span);
        if (!Data.empty() && xmod < Data.back().x)
            Data.shrink(0);
        Data.push_back(ImVec2(xmod, y));
    }
};



	class CustomLayoutNode {
	public:
		CustomLayoutNode(bool isVertical, const char* label = nullptr);
		CustomLayoutNode(CustomWindowFunc func, const char* label);
		~CustomLayoutNode();

		void AddVerticalChild(CustomLayoutNode* child);
		void AddHorizontalChild(CustomLayoutNode* child);
		void SetHorizontalChildren(CustomLayoutNode* left, CustomLayoutNode* middle = nullptr, CustomLayoutNode* right = nullptr);
		void SetVerticalChildren(CustomLayoutNode* left, CustomLayoutNode* middle = nullptr, CustomLayoutNode* right = nullptr);

		const std::string& GetLabel() const;
		void SetLabel(const char* label);

		void SetVisible(bool v);
		bool IsVisibleFlag() const;
		bool IsEffectivelyVisible() const;
		void SetDFSVisible(bool new_status);

		void ResizeNodeAndChildren(ImVec2 newPos, ImVec2 newSize);
		void RenderNodeAndChildren();
		bool FindHoveredSplitter(const ImVec2& mousePos, CustomLayoutNode*& outNode, int& outBoundaryIndex);
		bool HandleSplitterDragAt(int boundaryIndex, const ImVec2& mouseDelta);

		int VisibleChildCount() const;
		bool IsWindowNode() const;
		bool IsVerticalSplitter() const;
		bool IsHorizontalSplitter() const;

		const std::vector<CustomLayoutNode*>& GetChildren() const;
		std::vector<CustomLayoutNode*>& GetChildren();

		void EqualizeIfVisibleCountChanged();
		ImVec2 GetDomainPos() const;
		ImVec2 GetDomainSize() const;

	private:
		bool m_isVertical = false;
		std::string m_label;
		bool m_visible = true;
		ImVec2 m_domainPos{ 0,0 };
		ImVec2 m_domainSize{ 0,0 };
		std::vector<CustomLayoutNode*> m_children;
		std::vector<float> m_splitRatios;
		float m_splitterWidth = 4.0f;
		float m_minRatio = 0.05f;
		size_t m_lastVisibleCount = 0;
		bool m_equalizeOnVisibleChange = true;
		CustomWindowFunc m_pfnCustomWindowFunc = nullptr;

		static inline float Clamp01(float v);
	};

	class CustomLayout {
	public:
		explicit CustomLayout(CustomLayoutNode* root);
		~CustomLayout();

		void UpdateAndRender();
		CustomLayoutNode* GetRoot();

	private:
		void DrawControlPanel();

		CustomLayoutNode* m_root = nullptr;
		ImVec2 m_lastViewportSize{ 0,0 };
		CustomLayoutNode* m_activeNode = nullptr;
		int m_activeBoundaryIndex = -1;
	};

} // namespace FrameGUILayout