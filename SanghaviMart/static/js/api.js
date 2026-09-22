// SanghaviMart Client API SDK — v1 standard + legacy compat.
// New code MUST use apiV1 (/api/v1, {success,data,error}). Legacy `api` kept
// for backward compatibility with existing vanilla pages.
const API_V1 = '/api/v1';

/// XSS: escape ALL user-controlled strings before injecting into HTML.
/// Use escapeHtml(p.name), escapeHtml(p.description), etc. Never use raw
/// product names, reviews, usernames, or chatbot output in innerHTML.
function escapeHtml(s) {
  return String(s == null ? '' : s)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#39;');
}

async function v1request(endpoint, options = {}) {
  const res = await fetch(`${API_V1}${endpoint}`, {
    ...options,
    headers: { 'Content-Type': 'application/json', ...(options.headers || {}) },
    credentials: 'same-origin',
  });
  const data = await res.json().catch(() => ({}));
  if (!res.ok || data.success === false) {
    throw new Error((data.error && data.error.message) || `HTTP ${res.status}`);
  }
  return data.data;
}

const apiV1 = {
  // Auth (server-side sessions)
  register: (p) => v1request('/auth/register', { method: 'POST', body: JSON.stringify(p) }),
  login: (p) => v1request('/auth/login', { method: 'POST', body: JSON.stringify(p) }),
  logout: () => v1request('/auth/logout', { method: 'POST' }),
  me: () => v1request('/auth/me'),
  health: () => v1request('/health'),
  chat: (message) => v1request('/chat', { method: 'POST', body: JSON.stringify({ message }) }),
  // Products (price_cents integer minor units)
  products: (params = {}) => {
    const q = new URLSearchParams(params).toString();
    return v1request(`/products${q ? '?' + q : ''}`);
  },
  product: (id) => v1request(`/products/${encodeURIComponent(id)}`),
  createProduct: (p) => v1request('/products', { method: 'POST', body: JSON.stringify(p) }),
  updateProduct: (id, p) => v1request(`/products/${encodeURIComponent(id)}`, { method: 'PUT', body: JSON.stringify(p) }),
  deleteProduct: (id) => v1request(`/products/${encodeURIComponent(id)}`, { method: 'DELETE' }),
  // Cart
  cart: () => v1request('/cart'),
  cartAdd: (product_id, quantity = 1) => v1request('/cart', { method: 'POST', body: JSON.stringify({ product_id, quantity }) }),
  cartSet: (productId, quantity) => v1request(`/cart/${encodeURIComponent(productId)}`, { method: 'PUT', body: JSON.stringify({ quantity }) }),
  cartRemove: (productId) => v1request(`/cart/${encodeURIComponent(productId)}`, { method: 'DELETE' }),
  cartClear: () => v1request('/cart', { method: 'DELETE' }),
  // Orders
  checkout: () => v1request('/orders/checkout', { method: 'POST', body: '{}' }),
  orders: () => v1request('/orders'),
  order: (id) => v1request(`/orders/${encodeURIComponent(id)}`),
  orderStatus: (id, status) => v1request(`/orders/${encodeURIComponent(id)}/status`, { method: 'PUT', body: JSON.stringify({ status }) }),
  // Reviews
  review: (r) => v1request('/reviews', { method: 'POST', body: JSON.stringify(r) }),
  reviews: (pid) => v1request(`/products/${encodeURIComponent(pid)}/reviews`),
  // Admin
  adminUsers: (role = '') => v1request(`/admin/users${role ? '?role=' + encodeURIComponent(role) : ''}`),
  adminOrders: () => v1request('/admin/orders'),
  adminStats: () => v1request('/admin/stats'),
  adminDeleteProduct: (id) => v1request(`/admin/products/${encodeURIComponent(id)}`, { method: 'DELETE' }),
};

// Legacy — KEEP (existing pages depend on it).
const API_BASE = '/api';
const api = {
  getToken() { return localStorage.getItem('sm_token') || ''; },
  setToken(token) { localStorage.setItem('sm_token', token); },
  getUser() { const u = localStorage.getItem('sm_user'); return u ? JSON.parse(u) : null; },
  setUser(user) { localStorage.setItem('sm_user', JSON.stringify(user)); },
  clearAuth() { localStorage.removeItem('sm_token'); localStorage.removeItem('sm_user'); },
  async request(endpoint, options = {}) {
    const headers = { 'Content-Type': 'application/json', ...(options.headers || {}) };
    const token = this.getToken();
    if (token) headers['Authorization'] = `Bearer ${token}`;
    const resp = await fetch(`${API_BASE}${endpoint}`, { ...options, headers });
    const data = await resp.json();
    if (!resp.ok) throw new Error(data.error || `HTTP Error ${resp.status}`);
    return data;
  },
  register(payload) { return this.request('/register', { method: 'POST', body: JSON.stringify(payload) }); },
  login(payload) { return this.request('/login', { method: 'POST', body: JSON.stringify(payload) }); },
  logout() { this.clearAuth(); return this.request('/logout', { method: 'POST' }); },
  getMe() { return this.request('/auth/me'); },
  getProducts(params = {}) { const q = new URLSearchParams(params).toString(); return this.request(`/products${q ? '?' + q : ''}`); },
  getProductById(id) { return this.request(`/products/${id}`); },
  addProduct(p) { return this.request('/products', { method: 'POST', body: JSON.stringify(p) }); },
  updateProduct(id, p) { return this.request(`/products/${id}`, { method: 'PUT', body: JSON.stringify(p) }); },
  deleteProduct(id) { return this.request(`/products/${id}`, { method: 'DELETE' }); },
  getCategories() { return this.request('/categories'); },
  getCart() { return this.request('/cart'); },
  addToCart(productId, quantity = 1) { return this.request('/cart', { method: 'POST', body: JSON.stringify({ product_id: productId, quantity }) }); },
  updateCartItem(itemId, quantity) { return this.request(`/cart/${itemId}`, { method: 'PUT', body: JSON.stringify({ quantity }) }); },
  removeCartItem(itemId) { return this.request(`/cart/${itemId}`, { method: 'DELETE' }); },
  clearCart() { return this.request('/cart', { method: 'DELETE' }); },
  createOrder(d) { return this.request('/orders', { method: 'POST', body: JSON.stringify(d) }); },
  getOrders() { return this.request('/orders'); },
  getOrderById(id) { return this.request(`/orders/${id}`); },
  updateOrderStatus(orderId, status) { return this.request(`/orders/${orderId}/status`, { method: 'PUT', body: JSON.stringify({ status }) }); },
  addReview(r) { return this.request('/reviews', { method: 'POST', body: JSON.stringify(r) }); },
  getProductReviews(pid) { return this.request(`/products/${pid}/reviews`); },
  getAdminUsers(role = '') { return this.request(`/admin/users${role ? '?role=' + role : ''}`); },
  getAdminProducts() { return this.request('/admin/products'); },
  getAdminOrders() { return this.request('/admin/orders'); },
  getAdminStats() { return this.request('/admin/stats'); },
  adminDeleteProduct(id) { return this.request(`/admin/products/${id}`, { method: 'DELETE' }); },
  sendChatMessage(message) { return this.request('/chat', { method: 'POST', body: JSON.stringify({ message }) }); },
};
