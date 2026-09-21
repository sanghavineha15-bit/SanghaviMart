import React, { useState } from 'react';
import { Users, Package, ShoppingBag, DollarSign, Trash2, ShieldCheck, CheckCircle } from 'lucide-react';
import { User, Product, Order } from '../types';

interface AdminDashboardProps {
  users: User[];
  products: Product[];
  orders: Order[];
  onDeleteProduct: (productId: number) => void;
}

export const AdminDashboard: React.FC<AdminDashboardProps> = ({
  users,
  products,
  orders,
  onDeleteProduct,
}) => {
  const [activeTab, setActiveTab] = useState<'stats' | 'users' | 'products' | 'orders'>('stats');
  const [roleFilter, setRoleFilter] = useState<'all' | 'buyer' | 'seller'>('all');

  const totalRevenue = orders.reduce((sum, ord) => sum + ord.total_amount, 0);
  const filteredUsers = roleFilter === 'all' ? users : users.filter((u) => u.role === roleFilter);

  return (
    <div className="space-y-6">
      {/* Admin Header */}
      <div className="bg-slate-900 text-white p-6 rounded-2xl shadow-md flex flex-wrap items-center justify-between gap-4">
        <div>
          <div className="flex items-center gap-2 mb-1">
            <ShieldCheck className="w-6 h-6 text-purple-400" />
            <h1 className="text-xl font-extrabold">SanghaviMart Administrator Console</h1>
          </div>
          <p className="text-xs text-slate-300">
            Platform governance, user verification, catalog moderation, and real-time transaction oversight.
          </p>
        </div>

        <div className="text-xs bg-purple-950/60 border border-purple-800 text-purple-200 px-3 py-1.5 rounded-lg">
          Logged in as: <strong>admin@sanghavimart.com</strong> (Root Role)
        </div>
      </div>

      {/* Analytics KPI Bento Grid */}
      <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
        <div className="bg-white p-4 rounded-xl border border-slate-200 shadow-xs">
          <div className="flex items-center justify-between text-slate-500 mb-2">
            <span className="text-xs font-semibold">Registered Users</span>
            <Users className="w-4 h-4 text-blue-600" />
          </div>
          <div className="text-2xl font-black text-slate-900">{users.length}</div>
          <div className="text-[11px] text-slate-500 mt-1">
            {users.filter((u) => u.role === 'buyer').length} Buyers, {users.filter((u) => u.role === 'seller').length} Sellers
          </div>
        </div>

        <div className="bg-white p-4 rounded-xl border border-slate-200 shadow-xs">
          <div className="flex items-center justify-between text-slate-500 mb-2">
            <span className="text-xs font-semibold">Active Products</span>
            <Package className="w-4 h-4 text-emerald-600" />
          </div>
          <div className="text-2xl font-black text-slate-900">{products.filter((p) => p.is_active).length}</div>
          <div className="text-[11px] text-slate-500 mt-1">Across 4 verified categories</div>
        </div>

        <div className="bg-white p-4 rounded-xl border border-slate-200 shadow-xs">
          <div className="flex items-center justify-between text-slate-500 mb-2">
            <span className="text-xs font-semibold">Total Orders Processed</span>
            <ShoppingBag className="w-4 h-4 text-amber-600" />
          </div>
          <div className="text-2xl font-black text-slate-900">{orders.length}</div>
          <div className="text-[11px] text-slate-500 mt-1">
            {orders.filter((o) => o.status === 'delivered').length} Delivered, {orders.filter((o) => o.status === 'pending').length} Pending
          </div>
        </div>

        <div className="bg-white p-4 rounded-xl border border-slate-200 shadow-xs">
          <div className="flex items-center justify-between text-slate-500 mb-2">
            <span className="text-xs font-semibold">Gross Merchandise Value</span>
            <DollarSign className="w-4 h-4 text-purple-600" />
          </div>
          <div className="text-2xl font-black text-slate-900">${totalRevenue.toFixed(2)}</div>
          <div className="text-[11px] text-emerald-600 font-medium mt-1">100% automated settlement</div>
        </div>
      </div>

      {/* Tabs */}
      <div className="flex border-b border-slate-200 gap-4 text-xs font-bold">
        <button
          onClick={() => setActiveTab('stats')}
          className={`pb-3 border-b-2 transition-colors ${
            activeTab === 'stats' ? 'border-purple-600 text-purple-700' : 'border-transparent text-slate-500'
          }`}
        >
          Platform Analytics
        </button>
        <button
          onClick={() => setActiveTab('users')}
          className={`pb-3 border-b-2 transition-colors ${
            activeTab === 'users' ? 'border-purple-600 text-purple-700' : 'border-transparent text-slate-500'
          }`}
        >
          User Accounts ({users.length})
        </button>
        <button
          onClick={() => setActiveTab('products')}
          className={`pb-3 border-b-2 transition-colors ${
            activeTab === 'products' ? 'border-purple-600 text-purple-700' : 'border-transparent text-slate-500'
          }`}
        >
          Catalog Moderation ({products.length})
        </button>
        <button
          onClick={() => setActiveTab('orders')}
          className={`pb-3 border-b-2 transition-colors ${
            activeTab === 'orders' ? 'border-purple-600 text-purple-700' : 'border-transparent text-slate-500'
          }`}
        >
          All Orders ({orders.length})
        </button>
      </div>

      {/* Tab: Platform Analytics */}
      {activeTab === 'stats' && (
        <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
          <div className="bg-white p-5 rounded-2xl border border-slate-200 shadow-xs">
            <h3 className="text-sm font-bold text-slate-900 mb-3">Architecture & Database Health</h3>
            <div className="space-y-2 text-xs text-slate-600">
              <div className="flex justify-between py-1 border-b border-slate-100">
                <span>Database Engine:</span>
                <span className="font-semibold text-slate-800">PostgreSQL (Drogon Connection Pool)</span>
              </div>
              <div className="flex justify-between py-1 border-b border-slate-100">
                <span>C++ Thread Model:</span>
                <span className="font-semibold text-slate-800">16 Non-blocking Event Loop Threads</span>
              </div>
              <div className="flex justify-between py-1 border-b border-slate-100">
                <span>Password Protection:</span>
                <span className="font-semibold text-emerald-600">Salted SHA-256 / PBKDF2</span>
              </div>
              <div className="flex justify-between py-1 border-b border-slate-100">
                <span>Authentication Protocol:</span>
                <span className="font-semibold text-blue-600">JWT (HMAC-SHA256 Stateless Tokens)</span>
              </div>
              <div className="flex justify-between py-1">
                <span>Oversell Prevention:</span>
                <span className="font-semibold text-emerald-600">Active ACID DB Transactions</span>
              </div>
            </div>
          </div>

          <div className="bg-white p-5 rounded-2xl border border-slate-200 shadow-xs">
            <h3 className="text-sm font-bold text-slate-900 mb-3">Order Status Distribution</h3>
            <div className="space-y-2 text-xs">
              {(['delivered', 'shipped', 'processing', 'pending'] as const).map((st) => {
                const count = orders.filter((o) => o.status === st).length;
                const pct = orders.length > 0 ? (count / orders.length) * 100 : 0;
                return (
                  <div key={st}>
                    <div className="flex justify-between text-slate-600 font-medium mb-1">
                      <span className="uppercase text-[10px] tracking-wider">{st}</span>
                      <span>{count} ({pct.toFixed(0)}%)</span>
                    </div>
                    <div className="w-full h-2 bg-slate-100 rounded-full overflow-hidden">
                      <div
                        className={`h-full ${
                          st === 'delivered'
                            ? 'bg-emerald-500'
                            : st === 'shipped'
                            ? 'bg-blue-500'
                            : st === 'processing'
                            ? 'bg-purple-500'
                            : 'bg-amber-500'
                        }`}
                        style={{ width: `${pct}%` }}
                      />
                    </div>
                  </div>
                );
              })}
            </div>
          </div>
        </div>
      )}

      {/* Tab: Users */}
      {activeTab === 'users' && (
        <div className="bg-white rounded-2xl border border-slate-200 overflow-hidden shadow-xs">
          <div className="p-3 bg-slate-50 border-b border-slate-200 flex items-center justify-between">
            <div className="text-xs font-semibold text-slate-700">Filter By Role:</div>
            <div className="flex gap-1">
              {(['all', 'buyer', 'seller'] as const).map((r) => (
                <button
                  key={r}
                  onClick={() => setRoleFilter(r)}
                  className={`px-2.5 py-1 text-xs rounded-md font-medium transition-colors ${
                    roleFilter === r ? 'bg-purple-600 text-white font-bold' : 'text-slate-600 hover:bg-slate-200'
                  }`}
                >
                  {r.toUpperCase()}
                </button>
              ))}
            </div>
          </div>

          <div className="overflow-x-auto">
            <table className="w-full text-left text-xs">
              <thead className="bg-slate-50 text-slate-500 uppercase tracking-wider font-semibold border-b border-slate-200">
                <tr>
                  <th className="p-3">User ID</th>
                  <th className="p-3">Name</th>
                  <th className="p-3">Email Address</th>
                  <th className="p-3">Role</th>
                  <th className="p-3">Phone</th>
                  <th className="p-3">Address</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-100">
                {filteredUsers.map((u) => (
                  <tr key={u.id} className="hover:bg-slate-50/70">
                    <td className="p-3 font-mono font-semibold text-slate-500">#{u.id}</td>
                    <td className="p-3 font-semibold text-slate-900">{u.name}</td>
                    <td className="p-3 text-slate-600 font-mono">{u.email}</td>
                    <td className="p-3">
                      <span
                        className={`px-2 py-0.5 rounded-full text-[10px] font-bold uppercase ${
                          u.role === 'admin'
                            ? 'bg-purple-100 text-purple-800'
                            : u.role === 'seller'
                            ? 'bg-amber-100 text-amber-800'
                            : 'bg-blue-100 text-blue-800'
                        }`}
                      >
                        {u.role}
                      </span>
                    </td>
                    <td className="p-3 text-slate-500">{u.phone || '—'}</td>
                    <td className="p-3 text-slate-500 max-w-xs truncate">{u.address || '—'}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
      )}

      {/* Tab: Products */}
      {activeTab === 'products' && (
        <div className="bg-white rounded-2xl border border-slate-200 overflow-hidden shadow-xs">
          <div className="overflow-x-auto">
            <table className="w-full text-left text-xs">
              <thead className="bg-slate-50 text-slate-500 uppercase tracking-wider font-semibold border-b border-slate-200">
                <tr>
                  <th className="p-3">Product</th>
                  <th className="p-3">Seller</th>
                  <th className="p-3">Price</th>
                  <th className="p-3">Stock</th>
                  <th className="p-3">Rating</th>
                  <th className="p-3 text-right">Moderation Action</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-100">
                {products.map((p) => (
                  <tr key={p.id} className="hover:bg-slate-50/70">
                    <td className="p-3">
                      <div className="flex items-center gap-3">
                        <img src={p.image_url} alt={p.name} className="w-10 h-10 rounded-lg object-cover" />
                        <div>
                          <div className="font-semibold text-slate-900">{p.name}</div>
                          <div className="text-[10px] text-slate-400">{p.category_name}</div>
                        </div>
                      </div>
                    </td>
                    <td className="p-3 text-slate-600 font-medium">{p.seller_name}</td>
                    <td className="p-3 font-bold text-slate-900">${p.price.toFixed(2)}</td>
                    <td className="p-3 font-semibold text-slate-700">{p.stock_quantity}</td>
                    <td className="p-3 text-slate-600">⭐ {p.average_rating.toFixed(1)}</td>
                    <td className="p-3 text-right">
                      <button
                        onClick={() => {
                          if (confirm(`Administrator Action: Remove product '${p.name}' from marketplace?`)) {
                            onDeleteProduct(p.id);
                          }
                        }}
                        className="px-2.5 py-1 text-xs font-semibold text-red-600 hover:bg-red-50 rounded-md transition-colors"
                      >
                        Remove Listing
                      </button>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
      )}

      {/* Tab: Orders */}
      {activeTab === 'orders' && (
        <div className="bg-white rounded-2xl border border-slate-200 overflow-hidden shadow-xs">
          <div className="overflow-x-auto">
            <table className="w-full text-left text-xs">
              <thead className="bg-slate-50 text-slate-500 uppercase tracking-wider font-semibold border-b border-slate-200">
                <tr>
                  <th className="p-3">Order #</th>
                  <th className="p-3">Customer</th>
                  <th className="p-3">Total Amount</th>
                  <th className="p-3">Status</th>
                  <th className="p-3">Items</th>
                  <th className="p-3">Order Date</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-100">
                {orders.map((o) => (
                  <tr key={o.id} className="hover:bg-slate-50/70">
                    <td className="p-3 font-mono font-bold text-blue-600">{o.order_number}</td>
                    <td className="p-3 font-semibold text-slate-900">{o.buyer_name} ({o.shipping_city})</td>
                    <td className="p-3 font-extrabold text-slate-900">${o.total_amount.toFixed(2)}</td>
                    <td className="p-3">
                      <span className="px-2 py-0.5 rounded text-[10px] font-bold uppercase bg-slate-100 text-slate-700">
                        {o.status}
                      </span>
                    </td>
                    <td className="p-3 text-slate-600">{o.items.length} items</td>
                    <td className="p-3 text-slate-400">{o.created_at}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
      )}
    </div>
  );
};
