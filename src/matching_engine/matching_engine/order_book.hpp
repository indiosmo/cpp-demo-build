#ifndef MATCHING_ENGINE_ORDER_BOOK_HPP
#define MATCHING_ENGINE_ORDER_BOOK_HPP

#include "matching_engine/v3/order_book.hpp"
#include "matching_engine/v3/order_node.hpp"

/* Production order-book and node aliases. v1/v2/v3 subdirectory headers expose the historical iterations. */

namespace matching_engine {

using order_book = v3::flat_order_book;
using order_node = v3::order_node;

} // namespace matching_engine

#endif /* MATCHING_ENGINE_ORDER_BOOK_HPP */
