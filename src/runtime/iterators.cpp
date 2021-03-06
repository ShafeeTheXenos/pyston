// Copyright (c) 2014-2015 Dropbox, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "core/types.h"
#include "runtime/objmodel.h"
#include "runtime/types.h"

namespace pyston {
namespace {

static std::string next_str("next");

class BoxIteratorGeneric : public BoxIteratorImpl {
private:
    Box* iterator;
    Box* value;

public:
    BoxIteratorGeneric(Box* container) : BoxIteratorImpl(container), iterator(nullptr), value(nullptr) {
        if (container) {
            // TODO: this should probably call getPystonIter
            iterator = getiter(container);
            if (iterator)
                next();
            else
                *this = end();
        }
    }

    void next() override {
        assert(iterator);
        Box* hasnext = iterator->hasnextOrNullIC();
        if (hasnext) {
            if (hasnext->nonzeroIC()) {
                value = iterator->nextIC();
            } else {
                *this = end();
            }
        } else {
            try {
                value = iterator->nextIC();
            } catch (ExcInfo e) {
                if (e.matches(StopIteration))
                    *this = end();
                else
                    throw e;
            }
        }
    }

    Box* getValue() override { return value; }

    void gcHandler(GCVisitor* v) override {
        v->visitPotential(iterator);
        v->visitPotential(value);
    }

    bool isSame(const BoxIteratorImpl* _rhs) override {
        const BoxIteratorGeneric* rhs = (const BoxIteratorGeneric*)_rhs;
        return iterator == rhs->iterator && value == rhs->value;
    }

    static BoxIteratorGeneric end() { return BoxIteratorGeneric(nullptr); }
};

template <typename T> class BoxIteratorIndex : public BoxIteratorImpl {
private:
    T* obj;
    uint64_t index;

    static bool hasnext(BoxedList* o, uint64_t i) { return i < o->size; }
    static Box* getValue(BoxedList* o, uint64_t i) { return o->elts->elts[i]; }

    static bool hasnext(BoxedTuple* o, uint64_t i) { return i < o->elts.size(); }
    static Box* getValue(BoxedTuple* o, uint64_t i) { return o->elts[i]; }

    static bool hasnext(BoxedString* o, uint64_t i) { return i < o->s.size(); }
    static Box* getValue(BoxedString* o, uint64_t i) { return new BoxedString(std::string(1, o->s[i])); }

public:
    BoxIteratorIndex(T* obj) : BoxIteratorImpl(obj), obj(obj), index(0) {
        if (obj && !hasnext(obj, index))
            *this = end();
    }

    void next() override {
        if (!end().isSame(this)) {
            ++index;
            if (!hasnext(obj, index))
                *this = end();
        }
    }

    Box* getValue() override { return getValue(obj, index); }

    void gcHandler(GCVisitor* v) override { v->visitPotential(obj); }

    bool isSame(const BoxIteratorImpl* _rhs) override {
        const auto rhs = (const BoxIteratorIndex*)_rhs;
        return obj == rhs->obj && index == rhs->index;
    }

    static BoxIteratorIndex end() { return BoxIteratorIndex(nullptr); }
};
}

llvm::iterator_range<BoxIterator> BoxIterator::getRange(Box* container) {
    if (container->cls == list_cls) {
        using BoxIteratorList = BoxIteratorIndex<BoxedList>;
        BoxIterator begin(std::make_shared<BoxIteratorIndex<BoxedList>>((BoxedList*)container));
        static BoxIterator end(std::make_shared<BoxIteratorIndex<BoxedList>>(BoxIteratorList::end()));
        return llvm::iterator_range<BoxIterator>(std::move(begin), end);
    } else if (container->cls == tuple_cls) {
        using BoxIteratorTuple = BoxIteratorIndex<BoxedTuple>;
        BoxIterator begin(std::make_shared<BoxIteratorIndex<BoxedTuple>>((BoxedTuple*)container));
        static BoxIterator end(std::make_shared<BoxIteratorIndex<BoxedTuple>>(BoxIteratorTuple::end()));
        return llvm::iterator_range<BoxIterator>(std::move(begin), end);
    } else if (container->cls == str_cls) {
        using BoxIteratorString = BoxIteratorIndex<BoxedString>;
        BoxIterator begin(std::make_shared<BoxIteratorIndex<BoxedString>>((BoxedString*)container));
        static BoxIterator end(std::make_shared<BoxIteratorIndex<BoxedString>>(BoxIteratorString::end()));
        return llvm::iterator_range<BoxIterator>(std::move(begin), end);
    }
    BoxIterator begin(std::make_shared<BoxIteratorGeneric>(container));
    static BoxIterator end(std::make_shared<BoxIteratorGeneric>(BoxIteratorGeneric::end()));
    return llvm::iterator_range<BoxIterator>(std::move(begin), end);
}
}
