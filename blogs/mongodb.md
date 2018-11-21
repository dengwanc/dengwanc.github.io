## enable user auth
> https://medium.com/@raj_adroit/mongodb-enable-authentication-enable-access-control-e8a75a26d332

1 create admin user for get root access
2 create your_db user
3 add the security.authorization setting
4 restart mongod

> https://github.com/docker-library/mongo/issues/174
```
注意!!! 在 docker 里
MONGO_INITDB_ROOT_USERNAME
MONGO_INITDB_ROOT_PASSWORD
生效仅在
volumes:
    - ~/persistent-volumes/mongo:/data/db
第一次创建的时候工作
```