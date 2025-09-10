#include "AnkerValidator.hpp"

#include "wx/numformatter.h"

wxBEGIN_EVENT_TABLE(RichTextNumValidatorBase, wxValidator)
EVT_CHAR(RichTextNumValidatorBase::OnChar)
EVT_KILL_FOCUS(RichTextNumValidatorBase::OnKillFocus)
wxEND_EVENT_TABLE()

int RichTextNumValidatorBase::GetFormatFlags() const
{
	int flags = wxNumberFormatter::Style_None;
	if (m_style & wxNUM_VAL_THOUSANDS_SEPARATOR)
		flags |= wxNumberFormatter::Style_WithThousandsSep;
	if (m_style & wxNUM_VAL_NO_TRAILING_ZEROES)
		flags |= wxNumberFormatter::Style_NoTrailingZeroes;

	return flags;
}

void RichTextNumValidatorBase::SetWindow(wxWindow* win)
{
	wxValidator::SetWindow(win);

	if (wxDynamicCast(m_validatorWindow, wxRichTextCtrl))
		return;
}

void RichTextNumValidatorBase::GetCurrentValueAndInsertionPoint(wxString& val, int& pos) const
{
	wxRichTextCtrl* const control = wxDynamicCast(m_validatorWindow, wxRichTextCtrl);
	if (!control)
		return;

	val = control->GetValue();
	pos = control->GetInsertionPoint();

	long selFrom, selTo;
	control->GetSelection(&selFrom, &selTo);

	const long selLen = selTo - selFrom;
	if (selLen)
	{
		// Remove selected text because pressing a key would make it disappear.
		val.erase(selFrom, selLen);

		// And adjust the insertion point to have correct position in the new
		// string.
		if (pos > selFrom)
		{
			if (pos >= selTo)
				pos -= selLen;
			else
				pos = selFrom;
		}
	}
}

bool RichTextNumValidatorBase::IsMinusOk(const wxString& val, int pos) const
{
	// We need to know if we accept negative numbers at all.
	if (!CanBeNegative())
		return false;

	// Minus is only ever accepted in the beginning of the string.
	if (pos != 0)
		return false;

	// And then only if there is no existing minus sign there.
	if (!val.empty() && val[0] == '-')
		return false;

	// Notice that entering '-' can make our value invalid, for example if
	// we're limited to -5..15 range and the current value is 12, then the
	// new value would be (invalid) -12. We consider it better to let the
	// user do this because perhaps he is going to press Delete key next to
	// make it -2 and forcing him to delete 1 first would be unnatural.
	//
	// TODO: It would be nice to indicate that the current control contents
	//       is invalid (if it's indeed going to be the case) once
	//       wxValidator supports doing this non-intrusively.
	return true;
}

void RichTextNumValidatorBase::OnChar(wxKeyEvent& event)
{
	// By default we just validate this key so don't prevent the normal
	// handling from taking place.
	event.Skip();

	if (!m_validatorWindow)
		return;

	const int ch = event.GetUnicodeKey();
	if (ch == WXK_NONE)
	{
		// It's a character without any Unicode equivalent at all, e.g. cursor
		// arrow or function key, we never filter those.
		return;
	}

	if (ch < WXK_SPACE || ch == WXK_DELETE)
	{
		// Allow ASCII control characters and Delete.
		return;
	}

	if (event.GetModifiers() & ~wxMOD_SHIFT)
	{
		// Keys using modifiers other than Shift don't change the number, so
		// ignore them.
		return;
	}

	// Check if this character is allowed in the current state.
	wxString val;
	int pos;
	GetCurrentValueAndInsertionPoint(val, pos);

	// Minus is a special case because we can deal with it directly here, for
	// all the rest call the derived class virtual function.
	const bool ok = ch == '-' ? IsMinusOk(val, pos) : IsCharOk(val, pos, ch);
	if (!ok)
	{
		if (!wxValidator::IsSilent())
			wxBell();

		// Do not skip the event in this case, stop handling it here.
		event.Skip(false);
	}
}

void RichTextNumValidatorBase::OnKillFocus(wxFocusEvent& event)
{
	event.Skip();

	wxRichTextCtrl* const control = wxDynamicCast(m_validatorWindow, wxRichTextCtrl);
	if (!control)
		return;

	wxRichTextCtrl* const text = wxDynamicCast(m_validatorWindow, wxRichTextCtrl);
	const bool wasModified = text ? text->IsModified() : false;

	const wxString& value = control->GetValue();

	// If the control is currently empty and it hasn't been modified by the
	// user at all, leave it empty because just giving it focus and taking it
	// away again shouldn't change the value.
	if (value.empty() && !wasModified)
		return;

	const wxString& valueNorm = NormalizeString(value);
	if (control->GetValue() == valueNorm)
	{
		// Don't do anything at all if the value doesn't really change, even if
		// the control optimizes away the calls to ChangeValue() which don't
		// actually change it, it's easier to skip all the complications below
		// if we don't need to do anything.
		return;
	}

	control->ChangeValue(valueNorm);

	// When we changed the control value above, its "modified" status was reset
	// so we need to explicitly keep it marked as modified if it was so in the
	// first place.
	if (wasModified)
		text->MarkDirty();
}

wxString RichTextIntegerValidatorBase::ToString(LongestValueType value) const
{
	if (CanBeNegative())
	{
		return wxNumberFormatter::ToString(value, GetFormatFlags());
	}
	else
	{
		ULongestValueType uvalue = static_cast<ULongestValueType>(value);
		return wxNumberFormatter::ToString(uvalue, GetFormatFlags());
	}
}

bool RichTextIntegerValidatorBase::FromString(const wxString& s, LongestValueType* value) const
{
	if (CanBeNegative())
	{
		return wxNumberFormatter::FromString(s, value);
	}
	else
	{
		// Parse as unsigned to ensure we don't accept minus sign here.
		ULongestValueType uvalue;
		if (!wxNumberFormatter::FromString(s, &uvalue))
			return false;

		// This cast is lossless.
		*value = static_cast<LongestValueType>(uvalue);

		return true;
	}
}

bool RichTextIntegerValidatorBase::IsCharOk(const wxString& val, int pos, wxChar ch) const
{
	// We only accept digits here (remember that '-' is taken care of by the
	// base class already).
	if (ch < '0' || ch > '9')
		return false;

	// Check whether the value we'd obtain if we accepted this key passes some
    // basic checks.
	const wxString newval(GetValueAfterInsertingChar(val, pos, ch));
	LongestValueType value;
	if (!FromString(newval, &value))
		return false;

	// Also check that it doesn't have too many Integer digits.
	if (newval.length()  > m_max_digits)
		return false;

	// Accept anything that looks like a number here, notably do _not_ call
	// IsInRange() because this would prevent entering any digits in an
	// initially empty control limited to the values between "10" and "20".
	return true;
}

// ============================================================================
// RichTextFloatingPointValidatorBase implementation
// ============================================================================

wxString RichTextFloatingPointValidatorBase::ToString(LongestValueType value) const
{
	// When using factor > 1 we're going to show more digits than are
	// significant in the real value, so decrease precision to account for it.
	int precision = m_precision;
	if (precision && m_factor > 1)
	{
		precision -= static_cast<int>(log10(static_cast<double>(m_factor)));
		if (precision < 0)
			precision = 0;
	}

	return wxNumberFormatter::ToString(value * m_factor,
		precision,
		GetFormatFlags());
}

bool RichTextFloatingPointValidatorBase::FromString(const wxString& s, LongestValueType* value) const
{
	if (!wxNumberFormatter::FromString(s, value))
		return false;

	*value /= m_factor;

	return true;
}

bool RichTextFloatingPointValidatorBase::IsCharOk(const wxString& val, int pos, wxChar ch) const
{
	const wxChar separator = wxNumberFormatter::GetDecimalSeparator();
	if (ch == separator)
	{
		if (val.find(separator) != wxString::npos)
		{
			// There is already a decimal separator, can't insert another one.
			return false;
		}

		// Prepending a separator before the minus sign isn't allowed.
		if (pos == 0 && !val.empty() && val[0] == '-')
			return false;

		// Otherwise always accept it, adding a decimal separator doesn't
		// change the number value and, in particular, can't make it invalid.
		// OTOH the checks below might not pass because strings like "." or
		// "-." are not valid numbers so parsing them would fail, hence we need
		// to treat it specially here.
		return true;
	}

	// Must be a digit then.
	if (ch < '0' || ch > '9')
		return false;

	// Check whether the value we'd obtain if we accepted this key passes some
	// basic checks.
	const wxString newval(GetValueAfterInsertingChar(val, pos, ch));

	LongestValueType value;
	if (!FromString(newval, &value))
		return false;

	const size_t posSep = newval.find(separator);
	
	// Also check that it doesn't have too many Integer digits.
	if (posSep != wxString::npos) {
		if (posSep > m_max_digits) {
			return false;
		}
		else if (newval.length() - posSep - 1 > m_precision) {
			return false;
		}		
	}
	else if(newval.length() > m_max_digits) {
		return false;
	}

	// Also check that it doesn't have too many decimal digits.
	//if (posSep != wxString::npos && newval.length() - posSep - 1 > m_precision)
	//	return false;

	// Note that we do _not_ check if it's in range here, see the comment in
	// RichtextIntegerValidatorBase::IsCharOk().
	return true;
}
