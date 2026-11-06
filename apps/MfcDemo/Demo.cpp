#include <afxwin.h>
#include <afxcmn.h>
#include <afxdlgs.h>
#include <afxdialogex.h>
#include <atlimage.h>
#include <future>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <cstring>
#include "resource.h"
#include "AnalysisService.h"

using namespace glim::halcon;

class ImagePane : public CStatic
{
public:
	CImage m_image;
protected:
	afx_msg void OnPaint()
	{
		CPaintDC dc(this);
		CRect rect; GetClientRect(rect);
		dc.FillSolidRect(rect, RGB(24, 24, 24));
		if (m_image.IsNull()) return;
		const double scale = (std::min)(double(rect.Width()) / m_image.GetWidth(), double(rect.Height()) / m_image.GetHeight());
		const int width = int(m_image.GetWidth() * scale), height = int(m_image.GetHeight() * scale);
		m_image.Draw(dc, (rect.Width() - width) / 2, (rect.Height() - height) / 2, width, height);
	}
	DECLARE_MESSAGE_MAP()
};
BEGIN_MESSAGE_MAP(ImagePane, CStatic)
	ON_WM_PAINT()
END_MESSAGE_MAP()

class MainDialog : public CDialogEx
{
public:
	MainDialog() : CDialogEx(IDD_MAIN) {}
private:
	ImagePane m_original, m_mask;
	CListCtrl m_list;
	CComboBox m_mode;
	CString m_path;
	std::vector<unsigned char> m_pixels;
	std::future<std::shared_ptr<demo::Result>> m_pending;
	std::shared_ptr<demo::Result> m_result;
	bool m_busy = false;
	void Status(LPCTSTR text) { SetDlgItemText(IDC_STATUS, text); }
	void Busy(bool value)
	{
		m_busy = value;
		for (int id : {IDC_BTN_OPEN, IDC_BTN_RUN, IDC_EDIT_THRESHOLD, IDC_EDIT_AREA, IDC_EDIT_GAP, IDC_EDIT_OFFSET, IDC_MODE})
			GetDlgItem(id)->EnableWindow(!value);
	}
	bool Number(int id, double& value, double low, double high)
	{
		CString text; GetDlgItemText(id, text); text.Trim();
		char* end = nullptr; value = std::strtod(text.GetString(), &end);
		return !text.IsEmpty() && end && *end == 0 && std::isfinite(value) && value >= low && value <= high;
	}
	BOOL OnInitDialog() override
	{
		CDialogEx::OnInitDialog();
		m_original.SubclassDlgItem(IDC_ORIGINAL, this); m_mask.SubclassDlgItem(IDC_MASK, this);
		m_list.SubclassDlgItem(IDC_LIST_RESULTS, this); m_mode.SubclassDlgItem(IDC_MODE, this);
		m_mode.AddString("Blobs"); m_mode.AddString("Grouped defects"); m_mode.SetCurSel(0);
		m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
		const char* names[] = {"Index", "Parts", "Area(px)", "X", "Y", "Width", "Height", "Mean", "StdDev"};
		for (int i = 0; i < 9; ++i) m_list.InsertColumn(i, names[i], LVCFMT_LEFT, 85);
		SetDlgItemText(IDC_EDIT_THRESHOLD, "10"); SetDlgItemText(IDC_EDIT_AREA, "1");
		SetDlgItemText(IDC_EDIT_GAP, "12"); SetDlgItemText(IDC_EDIT_OFFSET, "4");
		return TRUE;
	}
	afx_msg void OnOpen()
	{
		try
		{
			CFileDialog dialog(TRUE, "bmp", nullptr, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, "BMP images (*.bmp)|*.bmp||", this);
			if (dialog.DoModal() != IDOK) return;
			m_path.Empty(); m_pixels.clear(); m_result.reset(); m_list.DeleteAllItems();
			m_original.m_image.Destroy(); m_mask.m_image.Destroy();
			m_original.Invalidate(); m_mask.Invalidate();
			if (FAILED(m_original.m_image.Load(dialog.GetPathName()))) { Status("Cannot load BMP preview."); return; }
			const auto pixels = static_cast<unsigned long long>(m_original.m_image.GetWidth()) * m_original.m_image.GetHeight();
			if (pixels == 0 || pixels > 16777216) { m_original.m_image.Destroy(); Status("Image exceeds 16,777,216 pixel limit."); return; }
			const int width = m_original.m_image.GetWidth(), height = m_original.m_image.GetHeight();
			CImage normalized;
			if (!normalized.Create(width, height, 24)) { Status("Preview conversion allocation failed."); return; }
			HDC target = normalized.GetDC();
			HDC source = m_original.m_image.GetDC();
			const BOOL drawn = ::BitBlt(target, 0, 0, width, height, source, 0, 0, SRCCOPY);
			m_original.m_image.ReleaseDC();
			normalized.ReleaseDC();
			if (!drawn) { Status("Preview conversion failed."); return; }
			m_pixels.resize(size_t(width) * height * 3);
			for (int y = 0; y < height; ++y)
				std::memcpy(m_pixels.data() + size_t(y) * width * 3, normalized.GetPixelAddress(0, y), size_t(width) * 3);
			m_original.m_image.Destroy(); m_original.m_image.Attach(normalized.Detach());
			m_original.Invalidate();
			m_path = dialog.GetPathName(); Status("BMP snapshot ready. Analyze to extract features.");
		}
		catch (CException* error) { error->Delete(); Status("MFC image load failed."); }
		catch (...) { Status("Image load failed."); }
	}
	afx_msg void OnRun()
	{
		if (m_busy || m_path.IsEmpty()) return;
		try
		{
			double contrast, area, gap, offset;
			if (!Number(IDC_EDIT_THRESHOLD, contrast, 0, 255) || !Number(IDC_EDIT_AREA, area, 1, 16777216) || std::floor(area) != area ||
				!Number(IDC_EDIT_GAP, gap, 0, 16777216) || !Number(IDC_EDIT_OFFSET, offset, 0, 16777216))
			{ Status("Invalid parameters: area must be a positive integer; other values nonnegative."); return; }
			ImageSettings image{}; DefectGroupingSettings grouping{};
			if (GetDefaultImageSettings(&image, sizeof(image)) != ImageStatus::Ok ||
				GetDefaultDefectGroupingSettings(&grouping, sizeof(grouping)) != ImageStatus::Ok)
			{ Status("SDK settings initialization failed."); return; }
			image.threshold = contrast; image.minArea = int(area);
			grouping.maxAlongGap = gap; grouping.maxAcrossOffset = offset;
			if (!SetTimer(1, 100, nullptr)) { Status("Cannot start completion timer."); return; }
			m_result.reset(); m_list.DeleteAllItems(); m_mask.m_image.Destroy(); m_mask.Invalidate();
			m_pending = std::async(std::launch::async, demo::Analyze, m_pixels, m_original.m_image.GetWidth(), m_original.m_image.GetHeight(), image, grouping);
			Busy(true); Status("Analyzing BMP on worker thread...");
		}
		catch (CException* error) { error->Delete(); KillTimer(1); Busy(false); Status("Cannot start analysis."); }
		catch (...) { KillTimer(1); Busy(false); Status("Cannot start analysis."); }
	}
	void Populate()
	{
		m_list.DeleteAllItems(); m_mask.m_image.Destroy(); m_mask.Invalidate();
		if (!m_result || m_result->status != ImageStatus::Ok) return;
		const bool grouped = m_mode.GetCurSel() == 1;
		const size_t count = grouped ? m_result->defectRows.size() : m_result->blobRows.size();
		for (size_t i = 0; i < count; ++i)
		{
			const auto& b = grouped ? BlobFeatures{} : m_result->blobRows[i];
			const auto& d = grouped ? m_result->defectRows[i] : DefectFeatures{};
			double values[] = {double(i), double(grouped ? d.memberCount : 1), double(grouped ? d.shape.area : b.shape.area),
				double(grouped ? d.x : b.x), double(grouped ? d.y : b.y), double(grouped ? d.width : b.width),
				double(grouped ? d.height : b.height), grouped ? d.meanGray : b.meanGray, grouped ? d.deviationGray : b.deviationGray};
			CString text; text.Format("%d", int(i)); m_list.InsertItem(int(i), text);
			for (int col = 1; col < 9; ++col) { text.Format(col < 7 ? "%.0f" : "%.3f", values[col]); m_list.SetItemText(int(i), col, text); }
		}
	}
	afx_msg void OnTimer(UINT_PTR id)
	{
		if (id != 1 || !m_pending.valid() || m_pending.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) return;
		KillTimer(1); Busy(false);
		try
		{
			m_result = m_pending.get(); Populate();
			CString text;
			if (m_result->status == ImageStatus::Ok) text.Format("Complete: %d blobs, %d grouped defects. Select a row for its mask.", int(m_result->blobRows.size()), int(m_result->defectRows.size()));
			else text.Format("Analysis failed: ImageStatus=%d (see SDK enum). No partial results displayed.", int(m_result->status));
			Status(text);
		}
		catch (CException* error) { error->Delete(); Status("Result display failed."); }
		catch (...) { Status("Analysis/result allocation failed."); }
	}
	afx_msg void OnMode()
	{
		try { Populate(); }
		catch (CException* error) { error->Delete(); Status("Result display failed."); }
		catch (...) { Status("Result display failed."); }
	}
	afx_msg void OnSelection(NMHDR* header, LRESULT* result)
	{
		*result = 0;
		const auto* change = reinterpret_cast<NMLISTVIEW*>(header);
		if (!(change->uChanged & LVIF_STATE) || !(change->uNewState & LVIS_SELECTED) || !m_result || m_result->status != ImageStatus::Ok) return;
		try
		{
			const int width = m_original.m_image.GetWidth(), height = m_original.m_image.GetHeight();
			std::vector<unsigned char> mask(size_t(width) * height);
			const ImageStatus status = m_mode.GetCurSel() == 1 ? CopyDefectMask(m_result->defects.get(), change->iItem, mask.data(), mask.size(), width) : CopyBlobMask(m_result->blobs.get(), change->iItem, mask.data(), mask.size(), width);
			m_mask.m_image.Destroy();
			if (status != ImageStatus::Ok) { m_mask.Invalidate(); Status("Mask copy failed. Check SDK result and buffer dimensions."); return; }
			if (!m_mask.m_image.Create(width, height, 24)) { Status("Mask allocation failed."); return; }
			for (int y = 0; y < height; ++y)
			{
				auto* row = static_cast<unsigned char*>(m_mask.m_image.GetPixelAddress(0, y));
				for (int x = 0; x < width; ++x) row[x * 3] = row[x * 3 + 1] = row[x * 3 + 2] = mask[size_t(y) * width + x];
			}
			m_mask.Invalidate();
		}
		catch (CException* error) { error->Delete(); Status("Mask display failed."); }
		catch (...) { Status("Mask allocation/display failed."); }
	}
	void OnOK() override { OnRun(); }
	void OnCancel() override { if (m_busy) Status("Wait for analysis before closing."); else CDialogEx::OnCancel(); }
	afx_msg void OnClose() { OnCancel(); }
	DECLARE_MESSAGE_MAP()
};
BEGIN_MESSAGE_MAP(MainDialog, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_OPEN, &MainDialog::OnOpen)
	ON_BN_CLICKED(IDC_BTN_RUN, &MainDialog::OnRun)
	ON_CBN_SELCHANGE(IDC_MODE, &MainDialog::OnMode)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_RESULTS, &MainDialog::OnSelection)
	ON_WM_TIMER()
	ON_WM_CLOSE()
END_MESSAGE_MAP()

class DemoApp : public CWinApp
{
	BOOL InitInstance() override
	{
		INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_LISTVIEW_CLASSES}; InitCommonControlsEx(&controls);
		CWinApp::InitInstance();
		try { MainDialog dialog; m_pMainWnd = &dialog; dialog.DoModal(); m_pMainWnd = nullptr; }
		catch (CException* error) { error->Delete(); AfxMessageBox("MFC initialization failed."); }
		catch (...) { AfxMessageBox("Application initialization failed."); }
		return FALSE;
	}
} application;
