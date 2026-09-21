import React, { useState } from 'react';
import { X, CheckCircle2, Truck, CreditCard, ShieldCheck } from 'lucide-react';
import { CartItem, Order } from '../types';

interface CheckoutModalProps {
  isOpen: boolean;
  onClose: () => void;
  cartItems: CartItem[];
  onConfirmOrder: (shippingDetails: {
    name: string;
    phone: string;
    address: string;
    city: string;
    postalCode: string;
    paymentMethod: string;
  }) => Order;
  onOrderSuccess: (order: Order) => void;
}

export const CheckoutModal: React.FC<CheckoutModalProps> = ({
  isOpen,
  onClose,
  cartItems,
  onConfirmOrder,
  onOrderSuccess,
}) => {
  const [shippingName, setShippingName] = useState('Neha Sanghavi');
  const [shippingPhone, setShippingPhone] = useState('+91 9123456780');
  const [shippingAddress, setShippingAddress] = useState('42 Lotus Residency, Link Road');
  const [shippingCity, setShippingCity] = useState('Mumbai');
  const [shippingPostalCode, setShippingPostalCode] = useState('400053');
  const [paymentMethod, setPaymentMethod] = useState('Cash on Delivery (Verified Demo)');
  const [completedOrder, setCompletedOrder] = useState<Order | null>(null);

  if (!isOpen) return null;

  const totalAmount = cartItems.reduce((acc, it) => acc + it.price * it.quantity, 0);

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    const order = onConfirmOrder({
      name: shippingName,
      phone: shippingPhone,
      address: shippingAddress,
      city: shippingCity,
      postalCode: shippingPostalCode,
      paymentMethod,
    });
    setCompletedOrder(order);
    onOrderSuccess(order);
  };

  const handleClose = () => {
    setCompletedOrder(null);
    onClose();
  };

  return (
    <div className="fixed inset-0 z-50 overflow-y-auto flex items-center justify-center p-4">
      <div className="fixed inset-0 bg-slate-900/60 backdrop-blur-xs" onClick={handleClose} />

      <div className="relative bg-white rounded-2xl max-w-xl w-full p-6 shadow-2xl overflow-hidden border border-slate-200">
        {completedOrder ? (
          /* Order Confirmation Success State */
          <div className="text-center py-6">
            <div className="w-16 h-16 bg-emerald-100 text-emerald-600 rounded-full flex items-center justify-center mx-auto mb-4 animate-bounce">
              <CheckCircle2 className="w-10 h-10" />
            </div>
            <h2 className="text-2xl font-black text-slate-900 mb-1">Order Placed Successfully!</h2>
            <p className="text-sm text-slate-600 mb-4">
              Thank you for shopping on SanghaviMart. Your order has been registered in the database.
            </p>

            <div className="bg-slate-50 border border-slate-200 rounded-xl p-4 text-left mb-6 space-y-2">
              <div className="flex justify-between text-xs">
                <span className="text-slate-500">Order Number:</span>
                <span className="font-mono font-bold text-blue-600">{completedOrder.order_number}</span>
              </div>
              <div className="flex justify-between text-xs">
                <span className="text-slate-500">Fulfillment Status:</span>
                <span className="bg-amber-100 text-amber-800 font-bold px-2 py-0.5 rounded text-[10px] uppercase">
                  {completedOrder.status}
                </span>
              </div>
              <div className="flex justify-between text-xs">
                <span className="text-slate-500">Total Amount:</span>
                <span className="font-bold text-slate-900">${completedOrder.total_amount.toFixed(2)}</span>
              </div>
              <div className="flex justify-between text-xs">
                <span className="text-slate-500">Deliver to:</span>
                <span className="text-slate-800 font-medium">{completedOrder.shipping_name}, {completedOrder.shipping_city}</span>
              </div>
              <div className="flex justify-between text-xs">
                <span className="text-slate-500">Payment:</span>
                <span className="text-slate-800 font-medium">{completedOrder.payment_method}</span>
              </div>
            </div>

            <div className="p-3 bg-blue-50 border border-blue-100 rounded-lg text-xs text-blue-800 mb-6 text-left">
              ✓ Automated inventory stock deducted from seller balances.<br />
              ✓ Invoice record generated for Seller & Admin inspection.<br />
              ✓ You can track this in <strong>My Orders</strong>.
            </div>

            <button
              onClick={handleClose}
              className="w-full py-2.5 px-4 bg-blue-600 hover:bg-blue-700 text-white text-xs font-bold rounded-xl transition-colors"
            >
              Continue Shopping
            </button>
          </div>
        ) : (
          /* Checkout Form */
          <div>
            <div className="flex items-center justify-between pb-4 mb-4 border-b border-slate-200">
              <div className="flex items-center gap-2">
                <Truck className="w-5 h-5 text-blue-600" />
                <h2 className="text-base font-bold text-slate-900">Checkout & Shipping Details</h2>
              </div>
              <button
                onClick={handleClose}
                className="p-1.5 text-slate-400 hover:text-slate-700 rounded-lg transition-colors"
              >
                <X className="w-5 h-5" />
              </button>
            </div>

            {/* Order Items Preview */}
            <div className="bg-slate-50 p-3 rounded-xl border border-slate-200 mb-4 max-h-36 overflow-y-auto space-y-2">
              <div className="text-[11px] font-bold text-slate-500 uppercase tracking-wider">
                Order Items ({cartItems.length})
              </div>
              {cartItems.map((it) => (
                <div key={it.id} className="flex justify-between text-xs items-center">
                  <span className="truncate pr-2 text-slate-700">{it.name} × {it.quantity}</span>
                  <span className="font-semibold text-slate-900 shrink-0">${(it.price * it.quantity).toFixed(2)}</span>
                </div>
              ))}
              <div className="pt-2 border-t border-slate-200 flex justify-between text-xs font-bold text-slate-900">
                <span>Total Due:</span>
                <span className="text-blue-600 text-sm">${totalAmount.toFixed(2)}</span>
              </div>
            </div>

            <form onSubmit={handleSubmit} className="space-y-3">
              <div className="grid grid-cols-2 gap-3">
                <div>
                  <label className="block text-xs font-medium text-slate-700 mb-1">Recipient Name</label>
                  <input
                    type="text"
                    value={shippingName}
                    onChange={(e) => setShippingName(e.target.value)}
                    required
                    className="w-full text-xs p-2 border border-slate-200 rounded-lg outline-none focus:border-blue-500"
                  />
                </div>
                <div>
                  <label className="block text-xs font-medium text-slate-700 mb-1">Phone Number</label>
                  <input
                    type="text"
                    value={shippingPhone}
                    onChange={(e) => setShippingPhone(e.target.value)}
                    required
                    className="w-full text-xs p-2 border border-slate-200 rounded-lg outline-none focus:border-blue-500"
                  />
                </div>
              </div>

              <div>
                <label className="block text-xs font-medium text-slate-700 mb-1">Delivery Street Address</label>
                <input
                  type="text"
                  value={shippingAddress}
                  onChange={(e) => setShippingAddress(e.target.value)}
                  required
                  className="w-full text-xs p-2 border border-slate-200 rounded-lg outline-none focus:border-blue-500"
                />
              </div>

              <div className="grid grid-cols-2 gap-3">
                <div>
                  <label className="block text-xs font-medium text-slate-700 mb-1">City</label>
                  <input
                    type="text"
                    value={shippingCity}
                    onChange={(e) => setShippingCity(e.target.value)}
                    required
                    className="w-full text-xs p-2 border border-slate-200 rounded-lg outline-none focus:border-blue-500"
                  />
                </div>
                <div>
                  <label className="block text-xs font-medium text-slate-700 mb-1">Postal Code</label>
                  <input
                    type="text"
                    value={shippingPostalCode}
                    onChange={(e) => setShippingPostalCode(e.target.value)}
                    required
                    className="w-full text-xs p-2 border border-slate-200 rounded-lg outline-none focus:border-blue-500"
                  />
                </div>
              </div>

              <div>
                <label className="block text-xs font-medium text-slate-700 mb-1">Payment Method</label>
                <select
                  value={paymentMethod}
                  onChange={(e) => setPaymentMethod(e.target.value)}
                  className="w-full text-xs p-2 border border-slate-200 rounded-lg outline-none focus:border-blue-500 bg-white"
                >
                  <option value="Cash on Delivery (Verified Demo)">Cash on Delivery (Simulated Instant Verification)</option>
                  <option value="Demo Credit/Debit Card">Demo UPI / Credit Card (Sandbox Mode)</option>
                </select>
              </div>

              <div className="pt-3">
                <button
                  type="submit"
                  className="w-full py-2.5 px-4 bg-blue-600 hover:bg-blue-700 text-white text-xs font-bold rounded-xl flex items-center justify-center gap-2 shadow-xs transition-colors"
                >
                  <ShieldCheck className="w-4 h-4" />
                  Confirm & Place Order (${totalAmount.toFixed(2)})
                </button>
              </div>
            </form>
          </div>
        )}
      </div>
    </div>
  );
};
