import React, { useState } from 'react';
import { Plus, Edit2, Trash2, Package, DollarSign, TrendingUp, AlertCircle, Check, X } from 'lucide-react';
import { Product, Order, User, Category } from '../types';

interface SellerDashboardProps {
  seller: User;
  products: Product[];
  categories: Category[];
  orders: Order[];
  onAddProduct: (productData: Partial<Product>) => void;
  onUpdateProduct: (productId: number, productData: Partial<Product>) => void;
  onDeleteProduct: (productId: number) => void;
  onUpdateOrderStatus: (orderId: number, status: Order['status']) => void;
}

export const SellerDashboard: React.FC<SellerDashboardProps> = ({
  seller,
  products,
  categories,
  orders,
  onAddProduct,
  onUpdateProduct,
  onDeleteProduct,
  onUpdateOrderStatus,
}) => {
  const [activeTab, setActiveTab] = useState<'inventory' | 'orders'>('inventory');
  const [isAddModalOpen, setIsAddModalOpen] = useState(false);
  const [editingProduct, setEditingProduct] = useState<Product | null>(null);

  // Form states
  const [name, setName] = useState('');
  const [description, setDescription] = useState('');
  const [price, setPrice] = useState(0);
  const [stock, setStock] = useState(10);
  const [categoryId, setCategoryId] = useState(1);
  const [imageUrl, setImageUrl] = useState('');

  const sellerProducts = products.filter((p) => p.seller_id === seller.id);
  const totalItemsSold = orders.reduce((acc, ord) => {
    return acc + ord.items.filter((it) => it.seller_id === seller.id).reduce((sub, it) => sub + it.quantity, 0);
  }, 0);
  const totalRevenue = orders.reduce((acc, ord) => {
    return acc + ord.items.filter((it) => it.seller_id === seller.id).reduce((sub, it) => sub + it.subtotal, 0);
  }, 0);

  const openAddModal = () => {
    setName('');
    setDescription('');
    setPrice(29.99);
    setStock(15);
    setCategoryId(categories[0]?.id || 1);
    setImageUrl('https://images.unsplash.com/photo-1546868871-7041f2a55e12?w=500&q=80');
    setEditingProduct(null);
    setIsAddModalOpen(true);
  };

  const openEditModal = (p: Product) => {
    setEditingProduct(p);
    setName(p.name);
    setDescription(p.description);
    setPrice(p.price);
    setStock(p.stock_quantity);
    setCategoryId(p.category_id);
    setImageUrl(p.image_url);
    setIsAddModalOpen(true);
  };

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    const cat = categories.find((c) => c.id === categoryId);
    const payload: Partial<Product> = {
      name,
      description,
      price: Number(price),
      stock_quantity: Number(stock),
      category_id: categoryId,
      category_name: cat?.name || 'General',
      image_url: imageUrl || 'https://images.unsplash.com/photo-1505740420928-5e560c06d30e?w=500&q=80',
      seller_id: seller.id,
      seller_name: seller.name,
      is_active: true,
    };

    if (editingProduct) {
      onUpdateProduct(editingProduct.id, payload);
    } else {
      onAddProduct(payload);
    }
    setIsAddModalOpen(false);
  };

  return (
    <div className="space-y-6">
      {/* Seller Header */}
      <div className="bg-white p-6 rounded-2xl border border-slate-200 shadow-xs flex flex-wrap items-center justify-between gap-4">
        <div>
          <div className="flex items-center gap-2 mb-1">
            <h1 className="text-xl font-extrabold text-slate-900">{seller.name}</h1>
            <span className="bg-amber-100 text-amber-800 text-[10px] font-bold px-2 py-0.5 rounded-full uppercase">
              Verified Seller
            </span>
          </div>
          <p className="text-xs text-slate-500">
            Manage your store catalog, real-time inventory levels, and customer order fulfillments.
          </p>
        </div>

        <button
          onClick={openAddModal}
          className="px-4 py-2 bg-blue-600 hover:bg-blue-700 text-white text-xs font-bold rounded-xl flex items-center gap-1.5 shadow-xs transition-colors"
        >
          <Plus className="w-4 h-4" />
          Add New Product
        </button>
      </div>

      {/* KPI Cards */}
      <div className="grid grid-cols-1 sm:grid-cols-3 gap-4">
        <div className="bg-white p-4 rounded-xl border border-slate-200">
          <div className="text-xs text-slate-500 font-medium">Active Listings</div>
          <div className="text-2xl font-black text-slate-900 mt-1">{sellerProducts.length}</div>
          <div className="text-[11px] text-emerald-600 mt-0.5">Live on SanghaviMart Catalog</div>
        </div>

        <div className="bg-white p-4 rounded-xl border border-slate-200">
          <div className="text-xs text-slate-500 font-medium">Total Units Sold</div>
          <div className="text-2xl font-black text-slate-900 mt-1">{totalItemsSold} units</div>
          <div className="text-[11px] text-blue-600 mt-0.5">Across all confirmed customer orders</div>
        </div>

        <div className="bg-white p-4 rounded-xl border border-slate-200">
          <div className="text-xs text-slate-500 font-medium">Gross Seller Revenue</div>
          <div className="text-2xl font-black text-slate-900 mt-1">${totalRevenue.toFixed(2)}</div>
          <div className="text-[11px] text-emerald-600 mt-0.5">Calculated in real time</div>
        </div>
      </div>

      {/* Tab Switcher */}
      <div className="flex border-b border-slate-200 gap-4 text-xs font-bold">
        <button
          onClick={() => setActiveTab('inventory')}
          className={`pb-3 border-b-2 transition-colors ${
            activeTab === 'inventory'
              ? 'border-blue-600 text-blue-600'
              : 'border-transparent text-slate-500 hover:text-slate-800'
          }`}
        >
          Inventory & Product Listings ({sellerProducts.length})
        </button>
        <button
          onClick={() => setActiveTab('orders')}
          className={`pb-3 border-b-2 transition-colors ${
            activeTab === 'orders'
              ? 'border-blue-600 text-blue-600'
              : 'border-transparent text-slate-500 hover:text-slate-800'
          }`}
        >
          Customer Orders Received
        </button>
      </div>

      {/* Tab 1: Inventory Table */}
      {activeTab === 'inventory' && (
        <div className="bg-white rounded-2xl border border-slate-200 overflow-hidden shadow-xs">
          <div className="overflow-x-auto">
            <table className="w-full text-left text-xs">
              <thead className="bg-slate-50 text-slate-500 font-semibold border-b border-slate-200 uppercase tracking-wider">
                <tr>
                  <th className="p-3">Product</th>
                  <th className="p-3">Category</th>
                  <th className="p-3">Price</th>
                  <th className="p-3">Stock Count</th>
                  <th className="p-3">Rating</th>
                  <th className="p-3 text-right">Actions</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-100">
                {sellerProducts.length === 0 ? (
                  <tr>
                    <td colSpan={6} className="p-8 text-center text-slate-500">
                      You have not listed any products yet. Click "Add New Product" above to publish your first listing.
                    </td>
                  </tr>
                ) : (
                  sellerProducts.map((p) => (
                    <tr key={p.id} className="hover:bg-slate-50/60 transition-colors">
                      <td className="p-3">
                        <div className="flex items-center gap-3">
                          <img
                            src={p.image_url}
                            alt={p.name}
                            className="w-10 h-10 rounded-lg object-cover bg-slate-100 border border-slate-200"
                          />
                          <div className="font-semibold text-slate-900 max-w-xs truncate">{p.name}</div>
                        </div>
                      </td>
                      <td className="p-3 text-slate-600">{p.category_name}</td>
                      <td className="p-3 font-bold text-slate-900">${p.price.toFixed(2)}</td>
                      <td className="p-3">
                        <span
                          className={`inline-block px-2 py-0.5 rounded text-[11px] font-bold ${
                            p.stock_quantity > 5
                              ? 'bg-emerald-100 text-emerald-800'
                              : p.stock_quantity > 0
                              ? 'bg-amber-100 text-amber-800'
                              : 'bg-red-100 text-red-800'
                          }`}
                        >
                          {p.stock_quantity} in stock
                        </span>
                      </td>
                      <td className="p-3 text-slate-600">
                        ⭐ {p.average_rating.toFixed(1)} ({p.review_count})
                      </td>
                      <td className="p-3 text-right">
                        <div className="flex items-center justify-end gap-2">
                          <button
                            onClick={() => openEditModal(p)}
                            className="p-1.5 text-slate-500 hover:text-blue-600 hover:bg-blue-50 rounded-lg transition-colors"
                            title="Edit Product"
                          >
                            <Edit2 className="w-4 h-4" />
                          </button>
                          <button
                            onClick={() => {
                              if (confirm(`Are you sure you want to delete '${p.name}'?`)) {
                                onDeleteProduct(p.id);
                              }
                            }}
                            className="p-1.5 text-slate-500 hover:text-red-600 hover:bg-red-50 rounded-lg transition-colors"
                            title="Delete Product"
                          >
                            <Trash2 className="w-4 h-4" />
                          </button>
                        </div>
                      </td>
                    </tr>
                  ))
                )}
              </tbody>
            </table>
          </div>
        </div>
      )}

      {/* Tab 2: Orders Received */}
      {activeTab === 'orders' && (
        <div className="space-y-4">
          {orders.filter((o) => o.items.some((it) => it.seller_id === seller.id)).length === 0 ? (
            <div className="bg-white p-8 rounded-2xl border border-slate-200 text-center text-slate-500 text-xs">
              No orders have been received for your products yet.
            </div>
          ) : (
            orders
              .filter((o) => o.items.some((it) => it.seller_id === seller.id))
              .map((order) => {
                const sellerItems = order.items.filter((it) => it.seller_id === seller.id);
                const sellerSubtotal = sellerItems.reduce((acc, it) => acc + it.subtotal, 0);

                return (
                  <div key={order.id} className="bg-white p-4 rounded-xl border border-slate-200 shadow-xs space-y-3">
                    <div className="flex flex-wrap items-center justify-between gap-2 border-b border-slate-100 pb-2.5">
                      <div>
                        <span className="font-mono font-bold text-xs text-slate-900">{order.order_number}</span>
                        <span className="text-xs text-slate-400 ml-2">Customer: {order.buyer_name} ({order.shipping_city})</span>
                      </div>

                      <div className="flex items-center gap-2">
                        <span className="text-xs text-slate-500">Status:</span>
                        <select
                          value={order.status}
                          onChange={(e) => onUpdateOrderStatus(order.id, e.target.value as Order['status'])}
                          className="text-xs font-semibold p-1 border border-slate-200 rounded-md bg-white outline-none focus:border-blue-500"
                        >
                          <option value="pending">Pending</option>
                          <option value="processing">Processing</option>
                          <option value="shipped">Shipped</option>
                          <option value="delivered">Delivered</option>
                          <option value="cancelled">Cancelled</option>
                        </select>
                      </div>
                    </div>

                    <div className="divide-y divide-slate-100 text-xs">
                      {sellerItems.map((it) => (
                        <div key={it.id} className="py-1.5 flex justify-between">
                          <span className="text-slate-700">{it.product_name} × {it.quantity}</span>
                          <span className="font-semibold text-slate-900">${it.subtotal.toFixed(2)}</span>
                        </div>
                      ))}
                    </div>

                    <div className="pt-2 border-t border-slate-100 flex justify-between text-xs font-bold text-slate-800">
                      <span>Seller Net Payable:</span>
                      <span className="text-emerald-600">${sellerSubtotal.toFixed(2)}</span>
                    </div>
                  </div>
                );
              })
          )}
        </div>
      )}

      {/* Add / Edit Product Modal */}
      {isAddModalOpen && (
        <div className="fixed inset-0 z-50 overflow-y-auto flex items-center justify-center p-4 bg-slate-900/60 backdrop-blur-xs">
          <div className="relative bg-white rounded-2xl max-w-lg w-full p-6 shadow-2xl border border-slate-200">
            <div className="flex items-center justify-between pb-3 mb-3 border-b border-slate-200">
              <h3 className="text-base font-bold text-slate-900">
                {editingProduct ? 'Edit Product Listing' : 'Add New Product Listing'}
              </h3>
              <button
                onClick={() => setIsAddModalOpen(false)}
                className="p-1.5 text-slate-400 hover:text-slate-700 rounded-lg"
              >
                <X className="w-5 h-5" />
              </button>
            </div>

            <form onSubmit={handleSubmit} className="space-y-3 text-xs">
              <div>
                <label className="block font-semibold text-slate-700 mb-1">Product Title</label>
                <input
                  type="text"
                  value={name}
                  onChange={(e) => setName(e.target.value)}
                  required
                  placeholder="e.g. Wireless Ergonomic Mouse"
                  className="w-full p-2 border border-slate-200 rounded-lg outline-none focus:border-blue-500"
                />
              </div>

              <div>
                <label className="block font-semibold text-slate-700 mb-1">Category</label>
                <select
                  value={categoryId}
                  onChange={(e) => setCategoryId(Number(e.target.value))}
                  className="w-full p-2 border border-slate-200 rounded-lg outline-none focus:border-blue-500 bg-white"
                >
                  {categories.map((c) => (
                    <option key={c.id} value={c.id}>
                      {c.name}
                    </option>
                  ))}
                </select>
              </div>

              <div className="grid grid-cols-2 gap-3">
                <div>
                  <label className="block font-semibold text-slate-700 mb-1">Price ($ USD)</label>
                  <input
                    type="number"
                    step="0.01"
                    min="0.01"
                    value={price}
                    onChange={(e) => setPrice(Number(e.target.value))}
                    required
                    className="w-full p-2 border border-slate-200 rounded-lg outline-none focus:border-blue-500"
                  />
                </div>
                <div>
                  <label className="block font-semibold text-slate-700 mb-1">Stock Quantity</label>
                  <input
                    type="number"
                    min="0"
                    value={stock}
                    onChange={(e) => setStock(Number(e.target.value))}
                    required
                    className="w-full p-2 border border-slate-200 rounded-lg outline-none focus:border-blue-500"
                  />
                </div>
              </div>

              <div>
                <label className="block font-semibold text-slate-700 mb-1">Image URL</label>
                <input
                  type="url"
                  value={imageUrl}
                  onChange={(e) => setImageUrl(e.target.value)}
                  placeholder="https://images.unsplash.com/photo-..."
                  className="w-full p-2 border border-slate-200 rounded-lg outline-none focus:border-blue-500"
                />
              </div>

              <div>
                <label className="block font-semibold text-slate-700 mb-1">Description & Specifications</label>
                <textarea
                  value={description}
                  onChange={(e) => setDescription(e.target.value)}
                  required
                  rows={3}
                  placeholder="Detailed specifications, warranty details, and box contents..."
                  className="w-full p-2 border border-slate-200 rounded-lg outline-none focus:border-blue-500"
                />
              </div>

              <div className="pt-2 flex justify-end gap-2">
                <button
                  type="button"
                  onClick={() => setIsAddModalOpen(false)}
                  className="px-4 py-2 border border-slate-200 text-slate-600 font-semibold rounded-lg hover:bg-slate-50"
                >
                  Cancel
                </button>
                <button
                  type="submit"
                  className="px-4 py-2 bg-blue-600 hover:bg-blue-700 text-white font-bold rounded-lg shadow-xs"
                >
                  {editingProduct ? 'Save Changes' : 'Publish Listing'}
                </button>
              </div>
            </form>
          </div>
        </div>
      )}
    </div>
  );
};
