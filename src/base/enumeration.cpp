/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#include "enumeration.h"
#include "qtcore_utils.h"

#include <cassert>

namespace Mayo {

Enumeration::Enumeration(std::initializer_list<Item> listItem)
    : m_vecItem(listItem)
{
}

Enumeration& Enumeration::chopPrefix(std::string_view prefix)
{
    for (Item& item : m_vecItem) {
        if (item.name.key.substr(0, prefix.size()) == prefix) // TODO Replace with C++20 starts_with()
            item.name.key = item.name.key.substr(prefix.size());
    }

    return *this;
}

Enumeration& Enumeration::changeTrContext(std::string_view context)
{
    for (Item& item : m_vecItem)
        item.name.trContext = context;

    return *this;
}

const Enumeration::Item& Enumeration::findItem(Enumeration::Value value) const
{
    const int index = this->findIndex(value);
    Expects(index != -1);
    return this->itemAt(index);
}

int Enumeration::findIndex(Value value) const
{
    auto it = std::find_if(
                m_vecItem.cbegin(),
                m_vecItem.cend(),
                [=](const Item& item) { return item.value == value; });
    return it != m_vecItem.cend() ? it - m_vecItem.cbegin() : -1;
}

Enumeration::Value Enumeration::findValue(std::string_view name) const
{
    const Enumeration::Item* ptrItem = this->findItem(name);
    assert(ptrItem != nullptr);
    return ptrItem ? ptrItem->value : -1;
}

bool Enumeration::contains(std::string_view name) const
{
    return this->findItem(name) != nullptr;
}

const Enumeration& Enumeration::null()
{
    static const Enumeration null;
    return null;
}

std::string_view Enumeration::findName(Value value) const
{
    const int index = this->findIndex(value);
    if (index != -1)
        return this->itemAt(index).name.key;

    return {};
}

const Enumeration::Item* Enumeration::findItem(std::string_view name) const
{
    auto itFound = std::find_if(
                m_vecItem.cbegin(),
                m_vecItem.cend(),
                [&](const Enumeration::Item& enumItem) { return name == enumItem.name.key; });
    return itFound != m_vecItem.cend() ? &(*itFound) : nullptr;
}

} // namespace Mayo
