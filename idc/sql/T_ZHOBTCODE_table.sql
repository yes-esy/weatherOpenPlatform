
-- ========== 安全删除部分（带异常处理）==========
BEGIN
   EXECUTE IMMEDIATE 'DROP INDEX IDX_ZHOBTCODE_1';
EXCEPTION 
   WHEN OTHERS THEN 
      IF SQLCODE != -1418 THEN RAISE; END IF;
END;
/

BEGIN
   EXECUTE IMMEDIATE 'DROP TABLE T_ZHOBTCODE CASCADE CONSTRAINTS';
EXCEPTION 
   WHEN OTHERS THEN 
      IF SQLCODE != -942 THEN RAISE; END IF;
END;
/

BEGIN
   EXECUTE IMMEDIATE 'DROP SEQUENCE SEQ_ZHOBT_CODE';
EXCEPTION 
   WHEN OTHERS THEN 
      IF SQLCODE != -2289 THEN RAISE; END IF;
END;
/

-- ========== 创建序列 ==========
CREATE SEQUENCE SEQ_ZHOBT_CODE
    INCREMENT BY 1
    MINVALUE 1
    NOCYCLE;

-- ========== 创建表 ==========
CREATE TABLE T_ZHOBTCODE
(
    site_id         CHAR(5)           NOT NULL,
    city_name       VARCHAR2(30)      NOT NULL,
    province_name   VARCHAR2(30)      NOT NULL,
    latitude        NUMBER(8)         NOT NULL,  
    longitude       NUMBER(8)         NOT NULL,
    height          NUMBER(8),
    update_time     DATE              DEFAULT SYSDATE NOT NULL,
    key_id          NUMBER(15)        NOT NULL,

    CONSTRAINT PK_ZHOBTCODE PRIMARY KEY (site_id)
        USING INDEX TABLESPACE INDEXS
)
TABLESPACE DATA;

-- ========== 添加注释 ==========
COMMENT ON TABLE T_ZHOBTCODE IS '该表存放了全国839个气象站点的基本信息。';

COMMENT ON COLUMN T_ZHOBTCODE.site_id        IS '站点代码，固定5个字符，一般是数字。';
COMMENT ON COLUMN T_ZHOBTCODE.city_name      IS '城市名称';
COMMENT ON COLUMN T_ZHOBTCODE.province_name  IS '省名称';
COMMENT ON COLUMN T_ZHOBTCODE.latitude       IS '纬度，单位：0.01度。';  -- 修正拼写
COMMENT ON COLUMN T_ZHOBTCODE.longitude      IS '经度，单位：0.01度。';
COMMENT ON COLUMN T_ZHOBTCODE.height         IS '海拔高度，单位：0.1米。';
COMMENT ON COLUMN T_ZHOBTCODE.update_time    IS '更新时间，数据被插入或更新的时间。';
COMMENT ON COLUMN T_ZHOBTCODE.key_id         IS '记录编号，从与本表同名的序列生成器中获取。';

-- ========== 创建唯一索引 ==========
CREATE UNIQUE INDEX IDX_ZHOBTCODE_1 ON T_ZHOBTCODE (key_id ASC)
    TABLESPACE INDEXS;



