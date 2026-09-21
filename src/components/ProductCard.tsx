import React from 'react';
import { Star, ShoppingCart, Check, AlertCircle } from 'lucide-react';
import { Product } from '../types';

interface ProductCardProps {
  product: Product;
  onAddToCart: (product: Product) => void;
  onViewReviews: (product: Product) => void;
}

export const ProductCard: React.FC<ProductCardProps> = ({
  product,
  onAddToCart,
  onViewReviews,
}) => {
  const isOutOfStock = product.stock_quantity <= 0;
  const isLowStock = product.stock_quantity > 0 && product.stock_quantity <= 5;

  return (
    <div className="bg-white rounded-xl border border-slate-200 overflow-hidden flex flex-col hover:shadow-md transition-all duration-200 group">
      {/* Product Image */}
      <div className="relative aspect-4/3 bg-slate-100 overflow-hidden">
        <img
          src={product.image_url}
          alt={product.name}
          className="w-full h-full object-cover group-hover:scale-105 transition-transform duration-300"
          loading="lazy"
        />
        <div className="absolute top-2.5 left-2.5">
          <span className="bg-white/95 backdrop-blur-xs text-blue-700 text-[11px] font-bold px-2 py-0.5 rounded-md shadow-xs border border-blue-100">
            {product.category_name}
          </span>
        </div>
        <div className="absolute top-2.5 right-2.5">
          <span className="bg-slate-900/80 backdrop-blur-xs text-white text-[11px] font-medium px-2 py-0.5 rounded-md">
            Sold by {product.seller_name}
          </span>
        </div>
      </div>

      {/* Card Content */}
      <div className="p-4 flex-1 flex flex-col">
        {/* Rating */}
        <button
          onClick={() => onViewReviews(product)}
          className="flex items-center gap-1.5 text-xs text-slate-500 mb-1.5 hover:text-blue-600 transition-colors self-start"
        >
          <div className="flex items-center text-amber-400">
            <Star className="w-3.5 h-3.5 fill-current" />
          </div>
          <span className="font-semibold text-slate-700">{product.average_rating.toFixed(1)}</span>
          <span>({product.review_count} reviews)</span>
        </button>

        {/* Title */}
        <h3 className="font-semibold text-slate-900 text-sm leading-snug line-clamp-2 mb-1.5">
          {product.name}
        </h3>

        {/* Description */}
        <p className="text-xs text-slate-500 line-clamp-2 mb-3 leading-relaxed">
          {product.description}
        </p>

        {/* Price & Stock Section */}
        <div className="mt-auto pt-3 border-t border-slate-100 flex items-center justify-between">
          <div>
            <div className="text-xs text-slate-400">Price</div>
            <div className="text-lg font-bold text-slate-900">${product.price.toFixed(2)}</div>
          </div>

          <div className="text-right">
            {isOutOfStock ? (
              <span className="inline-flex items-center gap-1 text-xs font-semibold text-red-600 bg-red-50 px-2 py-0.5 rounded-md">
                <AlertCircle className="w-3 h-3" />
                Out of Stock
              </span>
            ) : isLowStock ? (
              <span className="inline-flex items-center gap-1 text-xs font-semibold text-amber-700 bg-amber-50 px-2 py-0.5 rounded-md">
                Only {product.stock_quantity} left
              </span>
            ) : (
              <span className="inline-flex items-center gap-1 text-xs font-medium text-emerald-700 bg-emerald-50 px-2 py-0.5 rounded-md">
                <Check className="w-3 h-3" />
                {product.stock_quantity} in stock
              </span>
            )}
          </div>
        </div>

        {/* Action Button */}
        <button
          onClick={() => onAddToCart(product)}
          disabled={isOutOfStock}
          className={`mt-3 w-full py-2 px-3 rounded-lg text-xs font-semibold flex items-center justify-center gap-1.5 transition-colors ${
            isOutOfStock
              ? 'bg-slate-100 text-slate-400 cursor-not-allowed'
              : 'bg-blue-600 text-white hover:bg-blue-700 active:scale-[0.99]'
          }`}
        >
          <ShoppingCart className="w-3.5 h-3.5" />
          {isOutOfStock ? 'Currently Unavailable' : 'Add to Cart'}
        </button>
      </div>
    </div>
  );
};
