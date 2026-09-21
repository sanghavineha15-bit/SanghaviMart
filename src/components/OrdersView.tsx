import React, { useState } from 'react';
import { Package, Clock, Star, MessageSquare, CheckCircle, Truck } from 'lucide-react';
import { Order, Product, Review } from '../types';

interface OrdersViewProps {
  orders: Order[];
  products: Product[];
  onSubmitReview: (productId: number, rating: number, comment: string) => void;
}

export const OrdersView: React.FC<OrdersViewProps> = ({
  orders,
  products,
  onSubmitReview,
}) => {
  const [reviewingProductId, setReviewingProductId] = useState<number | null>(null);
  const [reviewRating, setReviewRating] = useState(5);
  const [reviewComment, setReviewComment] = useState('');
  const [reviewedProductIds, setReviewedProductIds] = useState<number[]>([]);

  const handleReviewSubmit = (productId: number) => {
    if (!reviewComment.trim()) {
      alert('Please write a comment for your review.');
      return;
    }
    onSubmitReview(productId, reviewRating, reviewComment);
    setReviewedProductIds([...reviewedProductIds, productId]);
    setReviewingProductId(null);
    setReviewComment('');
  };

  const getStatusBadge = (status: Order['status']) => {
    switch (status) {
      case 'delivered':
        return <span className="bg-emerald-100 text-emerald-800 text-[11px] font-bold px-2.5 py-0.5 rounded-full">Delivered</span>;
      case 'shipped':
        return <span className="bg-blue-100 text-blue-800 text-[11px] font-bold px-2.5 py-0.5 rounded-full">Shipped</span>;
      case 'processing':
        return <span className="bg-purple-100 text-purple-800 text-[11px] font-bold px-2.5 py-0.5 rounded-full">Processing</span>;
      default:
        return <span className="bg-amber-100 text-amber-800 text-[11px] font-bold px-2.5 py-0.5 rounded-full">Pending</span>;
    }
  };

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between pb-4 border-b border-slate-200">
        <div>
          <h2 className="text-xl font-extrabold text-slate-900">Order History & Invoices</h2>
          <p className="text-xs text-slate-500">Track fulfillment status and leave verified reviews on purchased items</p>
        </div>
        <span className="text-xs font-semibold text-blue-600 bg-blue-50 px-3 py-1 rounded-full border border-blue-200">
          {orders.length} Orders Recorded
        </span>
      </div>

      {orders.length === 0 ? (
        <div className="bg-white rounded-2xl border border-slate-200 p-12 text-center">
          <div className="w-16 h-16 bg-slate-100 text-slate-400 rounded-full flex items-center justify-center mx-auto mb-3">
            <Package className="w-8 h-8" />
          </div>
          <h3 className="font-bold text-slate-800 text-base mb-1">No Orders Found</h3>
          <p className="text-xs text-slate-500 max-w-sm mx-auto">
            You have not placed any orders yet. Browse the catalog, add items to your cart, and experience the C++ Drogon checkout pipeline!
          </p>
        </div>
      ) : (
        orders.map((order) => (
          <div
            key={order.id}
            className="bg-white rounded-2xl border border-slate-200 overflow-hidden shadow-xs hover:border-slate-300 transition-colors"
          >
            {/* Order Header */}
            <div className="p-4 bg-slate-50/80 border-b border-slate-200 flex flex-wrap items-center justify-between gap-3">
              <div className="flex items-center gap-3">
                <div className="w-9 h-9 bg-blue-100 text-blue-700 rounded-xl flex items-center justify-center font-bold text-xs">
                  ORD
                </div>
                <div>
                  <div className="flex items-center gap-2">
                    <span className="font-mono font-bold text-sm text-slate-900">{order.order_number}</span>
                    {getStatusBadge(order.status)}
                  </div>
                  <div className="text-[11px] text-slate-500 flex items-center gap-2 mt-0.5">
                    <Clock className="w-3 h-3" /> Placed on {order.created_at}
                  </div>
                </div>
              </div>

              <div className="text-right">
                <div className="text-xs text-slate-400">Total Invoice Amount</div>
                <div className="text-base font-extrabold text-slate-900">${order.total_amount.toFixed(2)}</div>
              </div>
            </div>

            {/* Order Shipping Summary */}
            <div className="px-4 py-2.5 bg-blue-50/40 border-b border-slate-100 text-xs text-slate-600 flex flex-wrap gap-4">
              <div><strong className="text-slate-800">Deliver To:</strong> {order.shipping_name}, {order.shipping_city}</div>
              <div><strong className="text-slate-800">Phone:</strong> {order.shipping_phone}</div>
              <div><strong className="text-slate-800">Payment:</strong> {order.payment_method}</div>
            </div>

            {/* Items Table / Cards */}
            <div className="p-4 divide-y divide-slate-100">
              {order.items.map((item) => {
                const isReviewing = reviewingProductId === item.product_id;
                const hasReviewed = reviewedProductIds.includes(item.product_id);

                return (
                  <div key={item.id} className="py-3 first:pt-0 last:pb-0 flex flex-col sm:flex-row sm:items-center justify-between gap-3">
                    <div className="flex items-center gap-3">
                      <img
                        src={item.image_url}
                        alt={item.product_name}
                        className="w-14 h-14 rounded-lg object-cover bg-slate-100 border border-slate-200 shrink-0"
                      />
                      <div>
                        <h4 className="font-semibold text-xs text-slate-900">{item.product_name}</h4>
                        <div className="text-[11px] text-slate-500">
                          Seller: <span className="font-medium text-slate-700">{item.seller_name}</span> | Qty: {item.quantity} × ${item.unit_price.toFixed(2)}
                        </div>
                        <div className="text-xs font-bold text-slate-800 mt-0.5">
                          Subtotal: ${item.subtotal.toFixed(2)}
                        </div>
                      </div>
                    </div>

                    {/* Review Button & Modal inline */}
                    <div className="flex items-center gap-2 shrink-0">
                      {hasReviewed ? (
                        <span className="inline-flex items-center gap-1 text-[11px] text-emerald-600 font-semibold bg-emerald-50 px-2.5 py-1 rounded-lg">
                          <CheckCircle className="w-3.5 h-3.5" /> Reviewed
                        </span>
                      ) : (
                        <button
                          onClick={() => setReviewingProductId(isReviewing ? null : item.product_id)}
                          className="px-3 py-1.5 text-xs font-medium text-blue-600 hover:text-blue-700 hover:bg-blue-50 border border-blue-200 rounded-lg flex items-center gap-1 transition-colors"
                        >
                          <Star className="w-3.5 h-3.5 fill-amber-400 text-amber-400" />
                          {isReviewing ? 'Cancel Review' : 'Write Verified Review'}
                        </button>
                      )}
                    </div>

                    {/* Inline Review Form */}
                    {isReviewing && (
                      <div className="w-full mt-3 p-3 bg-slate-50 rounded-xl border border-slate-200 space-y-2">
                        <div className="text-xs font-bold text-slate-800">
                          Rate & Review '{item.product_name}' (Verified Buyer)
                        </div>
                        <div className="flex items-center gap-1">
                          {[1, 2, 3, 4, 5].map((star) => (
                            <button
                              key={star}
                              type="button"
                              onClick={() => setReviewRating(star)}
                              className="p-1 text-slate-300 hover:text-amber-400"
                            >
                              <Star
                                className={`w-5 h-5 ${
                                  star <= reviewRating ? 'fill-amber-400 text-amber-400' : 'text-slate-300'
                                }`}
                              />
                            </button>
                          ))}
                          <span className="text-xs text-slate-600 font-semibold ml-2">
                            {reviewRating} of 5 Stars
                          </span>
                        </div>

                        <textarea
                          value={reviewComment}
                          onChange={(e) => setReviewComment(e.target.value)}
                          placeholder="Write your genuine feedback on quality, delivery, and performance..."
                          rows={2}
                          className="w-full text-xs p-2 bg-white border border-slate-200 rounded-lg outline-none focus:border-blue-500"
                        />

                        <button
                          onClick={() => handleReviewSubmit(item.product_id)}
                          className="px-4 py-1.5 bg-blue-600 text-white text-xs font-bold rounded-lg hover:bg-blue-700"
                        >
                          Submit Review
                        </button>
                      </div>
                    )}
                  </div>
                );
              })}
            </div>
          </div>
        ))
      )}
    </div>
  );
};
