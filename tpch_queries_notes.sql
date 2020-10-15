FULL TPCH Q6
SELECT SUM(l_extendedprice * l_discount) as revenue FROM lineitem WHERE l_shipdate >= '1994-01-01' AND l_shipdate < '1995-01-01' AND l_discount between 0.06 - 0.01 AND 0.06 + 0.01 AND l_quantity < 24; 

Fail on unsupported

Q6 without alias
SELECT SUM(l_extendedprice * l_discount) FROM lineitem WHERE l_shipdate >= '1994-01-01' AND l_shipdate < '1995-01-01' AND l_discount between 0.06 - 0.01 AND 0.06 + 0.01 AND l_quantity < 24; 
Needs OperatorType::Aggregate

Q6 without aggregate 
SELECT l_extendedprice * l_discount as revenue FROM lineitem WHERE l_shipdate >= '1994-01-01' AND l_shipdate < '1995-01-01' AND l_discount between 0.06 - 0.01 AND 0.06 + 0.01 AND l_quantity < 24; 
Needs OperatorType::Alias

Q6 without aggreagete, without allias
SELECT l_extendedprice * l_discount FROM lineitem WHERE l_shipdate >= '1994-01-01' AND l_shipdate < '1995-01-01' AND l_discount between 0.06 - 0.01 AND 0.06 + 0.01 AND l_quantity < 24; 
needs Failure. Unsupported ExpressionType::Arithmetic
AbstractLQPNode Type is: Predicate




Q6 without agg, alias and projection
SELECT * FROM lineitem WHERE l_shipdate >= '1994-01-01' AND l_shipdate < '1995-01-01' AND l_discount between 0.06 - 0.01 AND 0.06 + 0.01 AND l_quantity < 24; 
Works, does not support LQPNodeType::StoreTable
Do we need to support GetTable?
Not everything in projection.hpp is supported
In everything inhereting from AbstractOperator, we ignore lqp_node