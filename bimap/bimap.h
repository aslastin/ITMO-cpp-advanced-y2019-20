#pragma once

#include <cstddef>
#include <iostream>
#include <type_traits>
#include <random>
#include <utility>

template <typename Left, typename Right, typename CompareLeft = std::less<Left>, typename CompareRight = std::less<Right>>
struct bimap;

namespace intrusive {
    // To generate priorities
    std::mt19937 gen(std::random_device{}());

    struct tag_left;
    struct tag_right;

    struct node_base {
        node_base* left;
        node_base* right;
        node_base* parent;

        ~node_base() noexcept {
            clear();
        }

        void clear() {
            left = right = parent = nullptr;
        }
    };

    template <typename Tag>
    struct node : node_base {
        node() noexcept
            : node_base{nullptr, nullptr, nullptr}
        {}

        node(node_base* left, node_base* right, node_base* parent) noexcept
            : node_base{left, right, parent}
        {}

        node(node&& other) noexcept
            : node_base{other.left, other.right, other.parent}
        {
            other.clear();
        }

        node(node const&) = delete;
        node& operator=(node const&) = delete;

        template <typename Left, typename Right>
        friend struct map_element;

        template <typename T1, typename Tag1>
        friend node_base& to_base(T1&) noexcept;

        template <typename T1, typename Tag1>
        friend node_base const& to_base(T1 const&) noexcept;

        template <typename T1, typename Tag1>
        friend T1& from_base(node_base&) noexcept;

        template <typename T1, typename Tag1>
        friend T1 const& from_base(node_base const&) noexcept;
    };

    template <typename T, typename Tag>
    node_base& to_base(T& obj) noexcept {
        return static_cast<node<Tag>&>(obj);
    }

    template <typename T, typename Tag>
    node_base const& to_base(T const& obj) noexcept {
        return static_cast<node<Tag> const&>(obj);
    }

    template <typename T, typename Tag>
    T& from_base(node_base& base) noexcept {
        return static_cast<T&>(static_cast<node<Tag>&>(base));
    }

    template <typename T, typename Tag>
    T const& from_base(node_base const& base) noexcept {
        return static_cast<T const&>(static_cast<node<Tag> const&>(base));
    }

    template <typename Left, typename Right>
    struct map_element : node<tag_left>, node<tag_right> {
        map_element() noexcept
        {}

        template <typename L, typename R>
        map_element(L&& left, R&& right, int priority) noexcept
            : left(std::forward<L>(left)), right(std::forward<R>(right)), priority(priority)
        {}

        template <typename Key1, typename Value1, typename Tag1>
        friend struct map_iterator;

        template <typename Key1, typename Value1, typename Compare1, typename tag_t1>
        friend struct map;

        template <typename Left1, typename Right1, typename CompareLeft1, typename CompareRight1>
        friend struct ::bimap;
    private:
        Left left;
        Right right;
        int priority;
    };

    template <typename Key, typename Value, typename tag_t>
    struct map_iterator {
        using iterator = map_iterator<Key, Value, tag_t>;

        using element_t = std::conditional_t<std::is_same_v<tag_t, tag_left>, map_element<Key, Value>, map_element<Value, Key>>;

        using opposite_tag_t = std::conditional_t<std::is_same_v<tag_t, tag_left>, tag_right, tag_left>;
        using opposite_iterator = map_iterator<Value, Key, opposite_tag_t>;

        map_iterator()
            : current(nullptr)
        {}

        iterator& operator=(iterator const& other) {
            current = other.current;
            return *this;
        }

        // Элемент на который сейчас ссылается итератор.
        // Разыменование итератора end_left() неопределено.
        // Разыменование невалидного итератора неопределено.
        Key const& operator*() const {
            element_t const& el = from_base<element_t, tag_t>(*current);
            if constexpr (std::is_same_v<tag_t, tag_left>) {
                return el.left;
            } else {
                return el.right;
            }
        }

        // Переход к следующему по величине left'у.
        // Инкремент итератора end_left() неопределен.
        // Инкремент невалидного итератора неопределен.
        iterator& operator++() {
            if (current->right != nullptr) {
                current = current->right;
                while (current->left != nullptr) {
                    current = current->left;
                }
            } else {
                while (current->parent->left != current) {
                    current = current->parent;
                }
                current = current->parent;
            }
            return *this;
        }

        iterator operator++(int) {
            iterator copy = *this;
            ++*this;
            return copy;
        }

        // Переход к предыдущему по величине left'у.
        // Декремент итератора begin_left() неопределен.
        // Декремент невалидного итератора неопределен.
        iterator& operator--() {
            if (current->left != nullptr) {
                current = current->left;
                while (current->right != nullptr) {
                    current = current->right;
                }
            } else {
                while (current->parent->right != current) {
                    current = current->parent;
                }
                current = current->parent;
            }
            return *this;
        }

        iterator operator--(int) {
            iterator copy = *this;
            --*this;
            return copy;
        }

        // left_iterator ссылается на левый элемент некоторой пары.
        // Эта функция возвращает итератор на правый элемент той же пары.
        // end_left().flip() возращает end_right().
        // end_right().flip() возвращает end_left().
        // flip() невалидного итератора неопределен.
        opposite_iterator flip() const {
            return opposite_iterator(&to_base<element_t, opposite_tag_t>(from_base<element_t, tag_t>(*current)));
        }

        bool operator==(iterator const& rhs) const& noexcept {
            return current == rhs.current;
        }

        bool operator!=(iterator const& rhs) const& noexcept {
            return current != rhs.current;
        }
    private:
        explicit map_iterator(node_base const* current) noexcept
            : current(current)
        {}

        node_base const* get_data() const noexcept {
            return current;
        }
    private:
        node_base const* current;

        template <typename Key1, typename Value1, typename tag_t1>
        friend struct map_iterator;

        template <typename Key1, typename Value1, typename Compare1, typename Tag1>
        friend struct map;

        template <typename Left, typename Right, typename CompareLeft, typename CompareRight>
        friend struct ::bimap;
    };

    template <typename Key, typename Value, typename Compare, typename Tag>
    struct map : private Compare {
        using element_t = std::conditional_t<std::is_same_v<tag_left, Tag>, map_element<Key, Value>, map_element<Value, Key>>;
        using iterator = map_iterator<Key, Value, Tag>;

        map(Compare cmp = Compare()) noexcept
            : Compare(std::move(cmp)), fake{nullptr, nullptr, nullptr}
        {}

        map(map&& other) noexcept
            : Compare(std::move(other)), fake(other.fake)
        {
            other.fake.clear();
            upd_parent(fake.left, &fake);
        }

        map& operator=(map&& other) noexcept {
            if (this != &other) {
                static_cast<Compare&>(*this) = std::move(static_cast<Compare&>(other));
                fake = other.fake;
                other.fake.clear();
                upd_parent(fake.left, &fake);
            }
            return *this;
        }

        iterator insert(node_base* n) {
            insert_(fake.left, n);
            upd_parent(fake.left, &fake);
            return lower_bound(get_key_(n));
        }

        iterator erase(Key const& key) {
            erase_(fake.left, key);
            upd_parent(fake.left, &fake);
            return lower_bound(key);
        }

        iterator find(Key const& key) const {
            iterator res = lower_bound(key);
            return (res != end() && equals(key, *res)) ? res : end();
        }

        Value const& at(Key const& key) const {
            iterator it = find(key);
            if (it != end()) {
                return *it.flip();
            } else {
                throw std::out_of_range("No map_element with such key");
            }
        }

        iterator lower_bound(Key const& key) const {
            node_base const* current = fake.left;
            node_base const* res = &fake;
            while (current != nullptr) {
                Key const& k = get_key_(current);
                if (cmp(key, k) || equals(key, k)) {
                    res = current;
                    current = current->left;
                } else {
                    current = current->right;
                }
            }
            return iterator(res);
        }

        iterator upper_bound(Key const& key) const {
            node_base const* current = fake.left;
            node_base const* res = &fake;
            while (current != nullptr) {
                if (cmp(key, get_key_(current))) {
                    res = current;
                    current = current->left;
                } else {
                    current = current->right;
                }
            }
            return iterator(res);
        }

        iterator begin() const {
            node_base const* current = fake.left;
            if (current == nullptr) {
                return end();
            }
            while (current->left != nullptr) {
                current = current->left;
            }
            return iterator(current);
        }

        iterator end() const {
            return iterator(&fake);
        }

        bool equals(Key const& key1, Key const& key2) const {
            return !cmp(key1, key2) && !cmp(key2, key1);
        }

        void swap(map& other) noexcept {
            using std::swap;
            upd_parent(fake.left, &other.fake);
            upd_parent(other.fake.left, &fake);
            swap(fake, other.fake);
            swap(static_cast<Compare&>(*this), static_cast<Compare&>(other));
        }
    private:
        Compare const& get_cmp() const {
            return static_cast<Compare const&>(*this);
        }

        bool cmp(Key const& key1, Key const& key2) const {
            return get_cmp()(key1, key2);
        }

        Key const& get_key_(node_base const* const& n) const {
            element_t const& el = from_base<element_t, Tag>(*n);
            if constexpr (std::is_same_v<Tag, tag_left>) {
                return el.left;
            } else {
                return el.right;
            }
        }

        int get_priority_(node_base const* const& n) const {
            return from_base<element_t, Tag>(*n).priority;
        }

        void split_(node_base* t, Key const& key, node_base*& left, node_base*& right) {
            if (t == nullptr) {
                left = right = nullptr;
            } else {
                if (cmp(key, get_key_(t))) {
                    split_(t->left, key, left, t->left);
                    right = t;
                    upd_parent(right->left, right);
                } else {
                    split_(t->right, key, t->right, right);
                    left = t;
                    upd_parent(left->right, left);
                }
            }
        }

        void merge_(node_base*& t, node_base* left, node_base* right) {
            if (!left || !right) {
                t = left != nullptr ? left : right;
            } else {
                if (get_priority_(right) < get_priority_(left)) {
                    merge_(left->right, left->right, right);
                    t = left;
                    upd_parent(t->right, t);
                } else {
                    merge_(right->left, left, right->left);
                    t = right;
                    upd_parent(t->left, t);
                }
            }
        }

        // Инвариант: ключ у node уникальный
        void insert_(node_base*& t, node_base* n) {
            if (t == nullptr) {
                t = n;
            } else {
                n->parent = t->parent;
                if (get_priority_(n) > get_priority_(t)) {
                    split_(t, get_key_(n), n->left, n->right);
                    upd_parent(n->left, n);
                    upd_parent(n->right, n);
                    t = n;
                } else {
                    if (cmp(get_key_(n), get_key_(t))) {
                        insert_(t->left, n);
                        upd_parent(t->left, t);
                    } else {
                        insert_(t->right, n);
                        upd_parent(t->right, t);
                    }
                }
            }
        }

        // Инвариант: ключ существует
        void erase_(node_base*& t, Key const& key) {
            if (equals(key, get_key_(t))) {
                merge_(t, t->left, t->right);
            } else {
                if (cmp(key, get_key_(t))) {
                    erase_(t->left, key);
                    upd_parent(t->left, t);
                } else {
                    erase_(t->right, key);
                    upd_parent(t->right, t);
                }
            }
        }

        void upd_parent(node_base* t, node_base* p) {
            if (t != nullptr) {
                t->parent = p;
            }
        }
    private:
        node_base fake;

        template <typename Left, typename Right, typename CompareLeft, typename CompareRight>
        friend struct ::bimap;

        template <typename Left, typename Right, typename CompareLeft, typename CompareRight>
        friend bool operator==(::bimap<Left, Right, CompareLeft, CompareRight> const& a, ::bimap<Left, Right, CompareLeft, CompareRight> const& b);

        template <typename Key1, typename Value1, typename Compare1, typename tag_t1>
        friend struct map;
    };
}

template <typename Left, typename Right, typename CompareLeft, typename CompareRight>
struct bimap {
  using tag_left = intrusive::tag_left;
  using tag_right = intrusive::tag_right;
  using node_t = intrusive::map_element<Left, Right>;
  using node_base = intrusive::node_base;

  using left_iterator = intrusive::map_iterator<Left, Right, tag_left>;
  using right_iterator = intrusive::map_iterator<Right, Left, tag_right>;

  // Создает bimap не содержащий ни одной пары.
  bimap(CompareLeft compare_left = CompareLeft(), CompareRight compare_right = CompareRight()) noexcept
    : bimap(std::move(compare_left), std::move(compare_right), 0)
  {}

  // Конструкторы от других и присваивания
  bimap(bimap const& other)
    : bimap(other.map_left.get_cmp(), other.map_right.get_cmp())
  {
      sz = other.sz;
      left_iterator it = other.map_left.begin();
      while (it != other.end_left()) {
          node_t* nd = new node_t(*it, *it.flip(), other.map_left.get_priority_(it.get_data()));
          map_left.insert(&intrusive::to_base<node_t, tag_left>(*nd));
          map_right.insert(&intrusive::to_base<node_t, tag_right>(*nd));
          ++it;
      }
  }

  bimap(bimap&& other) noexcept
    : map_left(std::move(other.map_left)), map_right(std::move(other.map_right)), sz(other.sz)
  {
      other.sz = 0;
  }

  bimap& operator=(bimap const& other) {
      if (this != &other) {
          bimap tmp(other);
          swap(tmp);
      }
      return *this;
  }

  bimap& operator=(bimap&& other) noexcept {
      if (this != &other) {
          bimap tmp(std::move(other));
          swap(tmp);
      }
      return *this;
  }

  // Деструктор. Вызывается при удалении объектов bimap.
  // Инвалидирует все итераторы ссылающиеся на элементы этого bimap
  // (включая итераторы ссылающиеся на элементы следующие за последними).
  ~bimap() {
      for (left_iterator it = begin_left(); it != end_left();) {
          erase_left(it++);
      }
  }

  // Вставка пары (left, right), возвращает итератор на left.
  // Если такой left или такой right уже присутствуют в bimap, вставка не
  // производится и возвращается end_left().
  template <typename L = Left, typename R = Right>
  left_iterator insert(L&& left, R&& right) {
      if (find_left(left) == end_left() && find_right(right) == end_right()) {
          node_t* nd = new node_t(std::forward<L>(left), std::forward<R>(right), intrusive::gen());
          left_iterator it = map_left.insert(&intrusive::to_base<node_t, tag_left>(*nd));
          map_right.insert(&intrusive::to_base<node_t, tag_right>(*nd));
          ++sz;
          return it;
      } else {
          return end_left();
      }
  }

  // Удаляет элемент и соответствующий ему парный.
  // erase невалидного итератора неопределен.
  // erase(end_left()) и erase(end_right()) неопределены.
  // Пусть it ссылается на некоторый элемент e.
  // erase инвалидирует все итераторы ссылающиеся на e и на элемент парный к e.
  left_iterator erase_left(left_iterator it) {
      map_left.erase(*it);
      map_right.erase(*it.flip());
      auto res = map_left.lower_bound(*it);
      delete &intrusive::from_base<node_t, tag_left>(*it.get_data());
      --sz;
      return res;
  }
  // Аналогично erase, но по ключу, удаляет элемент если он присутствует, иначе
  // не делает ничего Возвращает была ли пара удалена
  bool erase_left(Left const& left) {
      left_iterator it;
      if ((it = find_left(left)) != end_left()) {
          erase_left(it);
          return true;
      } else {
          return false;
      }
  }

  right_iterator erase_right(right_iterator it) {
      map_right.erase(*it);
      map_left.erase(*it.flip());
      auto res = map_right.lower_bound(*it);
      delete &intrusive::from_base<node_t, tag_right>(*it.get_data());
      --sz;
      return res;
  }

  bool erase_right(Right const& right) {
      right_iterator it;
      if ((it = find_right(right)) != end_right()) {
          erase_right(it);
          return true;
      } else {
          return false;
      }
  }

  // erase от ренжа, удаляет [first, last), возвращает итератор на последний
  // элемент за удаленной последовательностью
  left_iterator erase_left(left_iterator first, left_iterator last) {
      for (left_iterator it = first; it != last;) {
          erase_left(it++);
      }
      return last;
  }

  right_iterator erase_right(right_iterator first, right_iterator last) {
      for (right_iterator it = first; it != last;) {
          erase_right(it++);
      }
      return last;
  }

  // Возвращает итератор по элементу. Если не найден - соответствующий end()
  left_iterator find_left(Left const& left) const {
      return map_left.find(left);
  }

  right_iterator find_right(Right const& right) const {
      return map_right.find(right);
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует -- бросает std::out_of_range
  Right const& at_left(Left const &key) const {
      return map_left.at(key);
  }

  Left const& at_right(Right const &key) const {
      return map_right.at(key);
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует, добавляет его в bimap и на противоположную
  // сторону кладет дефолтный элемент, ссылку на который и возвращает
  // Если дефолтный элемент уже лежит в противоположной паре - должен поменять
  // соответствующий ему элемент на запрашиваемый (смотри тесты)
  template <typename T = Right, std::enable_if_t<std::is_default_constructible_v<T>, bool> = true>
  Right const& at_left_or_default(Left const& key) {
      left_iterator left_it;
      if ((left_it = map_left.find(key)) == end_left()) {
          right_iterator right_it;
          Right r{};
          if ((right_it = map_right.find(r)) != end_right()) {
              erase_right(right_it);
          }
          return *insert(key, std::move(r)).flip();
      } else {
          return *left_it.flip();
      }
  }

  template <typename T = Left, std::enable_if_t<std::is_default_constructible_v<T>, bool> = true>
  Left const& at_right_or_default(Right const& key) {
      right_iterator right_it;
      if ((right_it = map_right.find(key)) == end_right()) {
          left_iterator left_it;
          Left l{};
          if ((left_it = map_left.find(l)) != end_left()) {
              erase_left(left_it);
          }
          return *insert(std::move(l), key);
      } else {
          return *right_it.flip();
      }
  }

  // lower и upper bound'ы по каждой стороне
  // Возвращают итераторы на соответствующие элементы
  // Смотри std::lower_bound, std::upper_bound.
  left_iterator lower_bound_left(Left const& left) const {
      return map_left.lower_bound(left);
  }

  left_iterator upper_bound_left(Left const& left) const {
      return map_left.upper_bound(left);
  }

  right_iterator lower_bound_right(Right const& right) const {
      return map_right.lower_bound(right);
  }

  right_iterator upper_bound_right(Right const& right) const {
      return map_right.upper_bound(right);
  }

  // Возващает итератор на минимальный по порядку left.
  left_iterator begin_left() const {
      return map_left.begin();
  }
  // Возващает итератор на следующий за последним по порядку left.
  left_iterator end_left() const {
      return map_left.end();
  }

  // Возващает итератор на минимальный по порядку right.
  right_iterator begin_right() const {
      return map_right.begin();
  }
  // Возващает итератор на следующий за последним по порядку right.
  right_iterator end_right() const {
      return map_right.end();
  }

  // Проверка на пустоту
  bool empty() const {
      return sz == 0;
  }

  // Возвращает размер бимапы (кол-во пар)
  std::size_t size() const {
      return sz;
  }

  void swap(bimap& other) noexcept {
      map_left.swap(other.map_left);
      map_right.swap(other.map_right);
      std::swap(sz, other.sz);
  }

  // операторы сравнения
  friend bool operator==(bimap const& a, bimap const& b) {
      if (a.sz != b.sz) {
          return false;
      }
      for (left_iterator it1 = a.begin_left(), it2 = b.begin_left(); it1 != a.end_left(); ++it1, ++it2) {
          if (!a.map_left.equals(*it1, *it2) || !a.map_right.equals(*it1.flip(), *it2.flip())) {
              return false;
          }
      }
      return true;
  }

  friend bool operator!=(bimap const &a, bimap const &b) {
      return !(a == b);
  }
private:
    bimap(CompareLeft&& compare_left, CompareRight&& compare_right, size_t sz) noexcept
        : map_left(std::move(compare_left)), map_right(std::move(compare_right)), sz(sz)
    {}

private:
    intrusive::map<Left, Right, CompareLeft, tag_left> map_left;
    intrusive::map<Right, Left, CompareRight, tag_right> map_right;
    size_t sz;

    template <typename Left1, typename Right1, typename CompareLeft1, typename CompareRight1>
    friend struct bimap;
};
