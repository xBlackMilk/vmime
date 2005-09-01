//
// VMime library (http://www.vmime.org)
// Copyright (C) 2002-2005 Vincent Richard <vincent@vincent-richard.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include "vmime/defaultParameter.hpp"
#include "vmime/parserHelpers.hpp"


namespace vmime
{


defaultParameter::defaultParameter()
	: m_value(vmime::create <word>())
{
}


defaultParameter& defaultParameter::operator=(const defaultParameter& other)
{
	copyFrom(other);
	return (*this);
}


const ref <const component> defaultParameter::getValueImp() const
{
	return (m_value);
}


const ref <component> defaultParameter::getValueImp()
{
	return (m_value);
}


const word& defaultParameter::getValue() const
{
	return (*m_value);
}


word& defaultParameter::getValue()
{
	return (*m_value);
}


void defaultParameter::setValue(const word& value)
{
	*m_value = value;
}


void defaultParameter::setValue(const component& value)
{
	const word& v = dynamic_cast <const word&>(value);
	*m_value = v;
}


void defaultParameter::parse(const string& buffer, const string::size_type position,
	const string::size_type end, string::size_type* newPosition)
{
	m_value = vmime::create <word>
		(string(buffer.begin() + position, buffer.begin() + end),
		 charset(charsets::US_ASCII));

	if (newPosition)
		*newPosition = end;
}


void defaultParameter::parse(const std::vector <valueChunk>& chunks)
{
	bool foundCharsetChunk = false;

	charset ch(charsets::US_ASCII);
	std::ostringstream value;

	for (std::vector <valueChunk>::size_type i = 0 ; i < chunks.size() ; ++i)
	{
		const valueChunk& chunk = chunks[i];

		// Decode following data
		if (chunk.encoded)
		{
			const string::size_type len = chunk.data.length();
			string::size_type pos = 0;

			// If this is the first encoded chunk, extract charset
			// and language information
			if (!foundCharsetChunk)
			{
				// Eg. "us-ascii'en'This%20is%20even%20more%20"
				string::size_type q = chunk.data.find_first_of('\'');

				if (q != string::npos)
				{
					const string chs = chunk.data.substr(0, q);

					if (!chs.empty())
						ch = charset(chs);

					++q;
					pos = q;
				}

				q = chunk.data.find_first_of('\'', pos);

				if (q != string::npos)
				{
					// Ignore language
					++q;
					pos = q;
				}

				foundCharsetChunk = true;
			}

			for (string::size_type i = pos ; i < len ; ++i)
			{
				const string::value_type c = chunk.data[i];

				if (c == '%' && i + 2 < len)
				{
					string::value_type v = 0;

					// First char
					switch (chunk.data[i + 1])
					{
					case 'a': case 'A': v += 10; break;
					case 'b': case 'B': v += 11; break;
					case 'c': case 'C': v += 12; break;
					case 'd': case 'D': v += 13; break;
					case 'e': case 'E': v += 14; break;
					case 'f': case 'F': v += 15; break;
					default: // assume 0-9

						v += (chunk.data[i + 1] - '0');
						break;
					}

					v *= 16;

					// Second char
					switch (chunk.data[i + 2])
					{
					case 'a': case 'A': v += 10; break;
					case 'b': case 'B': v += 11; break;
					case 'c': case 'C': v += 12; break;
					case 'd': case 'D': v += 13; break;
					case 'e': case 'E': v += 14; break;
					case 'f': case 'F': v += 15; break;
					default: // assume 0-9

						v += (chunk.data[i + 2] - '0');
						break;
					}

					value << v;

					i += 2; // skip next 2 chars
				}
				else
				{
					value << c;
				}
			}
		}
		// Simply copy data, as it is not encoded
		else
		{
			value << chunk.data;
		}
	}

	m_value = vmime::create <word>(value.str(), ch);
}


void defaultParameter::generate(utility::outputStream& os, const string::size_type maxLineLength,
	const string::size_type curLinePos, string::size_type* newLinePos) const
{
	const string& name = getName();
	const string& value = m_value->getBuffer();

	// For compatibility with implementations that do not understand RFC-2231,
	// also generate a normal "7bit/us-ascii" parameter
	string::size_type pos = curLinePos;

	if (pos + name.length() + 10 + value.length() > maxLineLength)
	{
		os << NEW_LINE_SEQUENCE;
		pos = NEW_LINE_SEQUENCE_LENGTH;
	}

	bool needQuoting = false;
	string::size_type valueLength = 0;

	for (string::size_type i = 0 ; (i < value.length()) && (pos + valueLength < maxLineLength - 4) ; ++i, ++valueLength)
	{
		switch (value[i])
		{
		// Characters that need to be quoted _and_ escaped
		case '"':
		case '\\':
		// Other characters that need quoting
		case ' ':
		case '\t':
		case '(':
		case ')':
		case '<':
		case '>':
		case '@':
		case ',':
		case ';':
		case ':':
		case '/':
		case '[':
		case ']':
		case '?':
		case '=':

			needQuoting = true;
			break;
		}
	}

	const bool cutValue = (valueLength != value.length());  // has the value been cut?

	if (needQuoting)
	{
		os << name << "=\"";
		pos += name.length() + 2;
	}
	else
	{
		os << name << "=";
		pos += name.length() + 1;
	}

	bool extended = false;

	for (string::size_type i = 0 ; (i < value.length()) && (pos < maxLineLength - 4) ; ++i)
	{
		const char_t c = value[i];

		if (/* needQuoting && */ (c == '"' || c == '\\'))  // 'needQuoting' is implicit
		{
			os << '\\' << value[i];  // escape 'x' with '\x'
			pos += 2;
		}
		else if (parserHelpers::isAscii(c))
		{
			os << value[i];
			++pos;
		}
		else
		{
			extended = true;
		}
	}

	if (needQuoting)
	{
		os << '"';
		++pos;
	}

	// Also generate an extended parameter if the value contains 8-bit characters
	// or is too long for a single line
	if (extended || cutValue)
	{
		os << ';';
		++pos;

		/* RFC-2231
		 * ========
		 *
		 * Content-Type: message/external-body; access-type=URL;
		 *    URL*0="ftp://";
		 *    URL*1="cs.utk.edu/pub/moore/bulk-mailer/bulk-mailer.tar"
		 *
		 * Content-Type: application/x-stuff;
		 *    title*=us-ascii'en-us'This%20is%20%2A%2A%2Afun%2A%2A%2A
		 *
		 * Content-Type: application/x-stuff;
		 *    title*0*=us-ascii'en'This%20is%20even%20more%20
		 *    title*1*=%2A%2A%2Afun%2A%2A%2A%20
		 *    title*2="isn't it!"
		 */

		// Check whether there is enough space for the first section:
		// parameter name, section identifier, charset and separators
		// + at least 5 characters for the value
		const string::size_type firstSectionLength =
			  name.length() + 4 /* *0*= */ + 2 /* '' */
			+ m_value->getCharset().getName().length();

		if (pos + firstSectionLength + 5 >= maxLineLength)
		{
			os << NEW_LINE_SEQUENCE;
			pos = NEW_LINE_SEQUENCE_LENGTH;
		}

		// Split text into multiple sections that fit on one line
		int sectionCount = 0;
		std::vector <string> sectionText;

		string currentSection;
		string::size_type currentSectionLength = firstSectionLength;

		for (string::size_type i = 0 ; i < value.length() ; ++i)
		{
			// Check whether we should start a new line (taking into
			// account the next character will be encoded = worst case)
			if (currentSectionLength + 3 >= maxLineLength)
			{
				sectionText.push_back(currentSection);
				sectionCount++;

				currentSection.clear();
				currentSectionLength = NEW_LINE_SEQUENCE_LENGTH
					+ name.length() + 6;
			}

			// Output next character
			const char_t c = value[i];
			bool encode = false;

			switch (c)
			{
			// special characters
			case ' ':
			case '\t':
			case '\r':
			case '\n':
			case '"':
			case ';':
			case ',':

				encode = true;
				break;

			default:

				encode = (!parserHelpers::isPrint(c) ||
				          !parserHelpers::isAscii(c));

				break;
			}

			if (encode)  // need encoding
			{
				const int h1 = static_cast <unsigned char>(c) / 16;
				const int h2 = static_cast <unsigned char>(c) % 16;

				currentSection += '%';
				currentSection += "0123456789ABCDEF"[h1];
				currentSection += "0123456789ABCDEF"[h2];

				pos += 3;
				currentSectionLength += 3;
			}
			else
			{
				currentSection += value[i];

				++pos;
				++currentSectionLength;
			}
		}

		if (!currentSection.empty())
		{
			sectionText.push_back(currentSection);
			sectionCount++;
		}

		// Output sections
		for (int sectionNumber = 0 ; sectionNumber < sectionCount ; ++sectionNumber)
		{
			os << name;

			if (sectionCount != 1) // no section specifier when only a single one
			{
				os << '*';
				os << sectionNumber;
			}

			os << "*=";

			if (sectionNumber == 0)
			{
				os << m_value->getCharset().getName();
				os << '\'' << /* No language */ '\'';
			}

			os << sectionText[sectionNumber];

			if (sectionNumber + 1 < sectionCount)
			{
				os << ';';
				os << NEW_LINE_SEQUENCE;
				pos = NEW_LINE_SEQUENCE_LENGTH;
			}
		}
	}

	if (newLinePos)
		*newLinePos = pos;
}



} // vmime