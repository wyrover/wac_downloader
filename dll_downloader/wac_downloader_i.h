

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Tue Oct 09 09:01:11 2012
 */
/* Compiler settings for wac_downloader.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 7.00.0555 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __wac_downloader_i_h__
#define __wac_downloader_i_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ICallBack_FWD_DEFINED__
#define __ICallBack_FWD_DEFINED__
typedef interface ICallBack ICallBack;
#endif 	/* __ICallBack_FWD_DEFINED__ */


#ifndef __IDownloadTask_FWD_DEFINED__
#define __IDownloadTask_FWD_DEFINED__
typedef interface IDownloadTask IDownloadTask;
#endif 	/* __IDownloadTask_FWD_DEFINED__ */


#ifndef __Idownloader2_FWD_DEFINED__
#define __Idownloader2_FWD_DEFINED__
typedef interface Idownloader2 Idownloader2;
#endif 	/* __Idownloader2_FWD_DEFINED__ */


#ifndef __downloader2_FWD_DEFINED__
#define __downloader2_FWD_DEFINED__

#ifdef __cplusplus
typedef class downloader2 downloader2;
#else
typedef struct downloader2 downloader2;
#endif /* __cplusplus */

#endif 	/* __downloader2_FWD_DEFINED__ */


#ifndef __CallBack_FWD_DEFINED__
#define __CallBack_FWD_DEFINED__

#ifdef __cplusplus
typedef class CallBack CallBack;
#else
typedef struct CallBack CallBack;
#endif /* __cplusplus */

#endif 	/* __CallBack_FWD_DEFINED__ */


#ifndef __DownloadTask_FWD_DEFINED__
#define __DownloadTask_FWD_DEFINED__

#ifdef __cplusplus
typedef class DownloadTask DownloadTask;
#else
typedef struct DownloadTask DownloadTask;
#endif /* __cplusplus */

#endif 	/* __DownloadTask_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __ICallBack_INTERFACE_DEFINED__
#define __ICallBack_INTERFACE_DEFINED__

/* interface ICallBack */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICallBack;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A7B5F82B-3707-4FFD-9F7D-1B9E12E6E73D")
    ICallBack : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnGetFileName( 
            /* [in] */ LONG TaskID,
            /* [in] */ BSTR szFileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnGetFileSize( 
            /* [in] */ LONG TaskID,
            /* [in] */ LONG nTotalSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnUpdate( 
            /* [in] */ LONG TaskID,
            /* [in] */ LONGLONG nCurPos,
            /* [in] */ LONGLONG nTotalSize,
            /* [in] */ LONG nSpeed) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnComplete( 
            /* [in] */ LONG nTaskID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnFail( 
            /* [in] */ LONG nTaskID,
            /* [in] */ LONG nsError) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnStart( 
            /* [in] */ LONG nTaskID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICallBackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICallBack * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICallBack * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICallBack * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnGetFileName )( 
            ICallBack * This,
            /* [in] */ LONG TaskID,
            /* [in] */ BSTR szFileName);
        
        HRESULT ( STDMETHODCALLTYPE *OnGetFileSize )( 
            ICallBack * This,
            /* [in] */ LONG TaskID,
            /* [in] */ LONG nTotalSize);
        
        HRESULT ( STDMETHODCALLTYPE *OnUpdate )( 
            ICallBack * This,
            /* [in] */ LONG TaskID,
            /* [in] */ LONGLONG nCurPos,
            /* [in] */ LONGLONG nTotalSize,
            /* [in] */ LONG nSpeed);
        
        HRESULT ( STDMETHODCALLTYPE *OnComplete )( 
            ICallBack * This,
            /* [in] */ LONG nTaskID);
        
        HRESULT ( STDMETHODCALLTYPE *OnFail )( 
            ICallBack * This,
            /* [in] */ LONG nTaskID,
            /* [in] */ LONG nsError);
        
        HRESULT ( STDMETHODCALLTYPE *OnStart )( 
            ICallBack * This,
            /* [in] */ LONG nTaskID);
        
        END_INTERFACE
    } ICallBackVtbl;

    interface ICallBack
    {
        CONST_VTBL struct ICallBackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICallBack_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICallBack_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICallBack_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICallBack_OnGetFileName(This,TaskID,szFileName)	\
    ( (This)->lpVtbl -> OnGetFileName(This,TaskID,szFileName) ) 

#define ICallBack_OnGetFileSize(This,TaskID,nTotalSize)	\
    ( (This)->lpVtbl -> OnGetFileSize(This,TaskID,nTotalSize) ) 

#define ICallBack_OnUpdate(This,TaskID,nCurPos,nTotalSize,nSpeed)	\
    ( (This)->lpVtbl -> OnUpdate(This,TaskID,nCurPos,nTotalSize,nSpeed) ) 

#define ICallBack_OnComplete(This,nTaskID)	\
    ( (This)->lpVtbl -> OnComplete(This,nTaskID) ) 

#define ICallBack_OnFail(This,nTaskID,nsError)	\
    ( (This)->lpVtbl -> OnFail(This,nTaskID,nsError) ) 

#define ICallBack_OnStart(This,nTaskID)	\
    ( (This)->lpVtbl -> OnStart(This,nTaskID) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICallBack_INTERFACE_DEFINED__ */


#ifndef __IDownloadTask_INTERFACE_DEFINED__
#define __IDownloadTask_INTERFACE_DEFINED__

/* interface IDownloadTask */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IDownloadTask;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8F6019BC-F276-4A75-BB7E-0AA9F075DF67")
    IDownloadTask : public IUnknown
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_nTaskID( 
            /* [retval][out] */ LONG *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_nTaskID( 
            /* [in] */ LONG newVal) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_nTotalLength( 
            /* [retval][out] */ LONGLONG *pVal) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_nTotalLength( 
            /* [in] */ LONGLONG newVal) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_nDownloadSize( 
            /* [retval][out] */ LONGLONG *pVal) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_nDownloadSize( 
            /* [in] */ LONGLONG newVal) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_fSpeed( 
            /* [retval][out] */ FLOAT *pVal) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_fSpeed( 
            /* [in] */ FLOAT pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDownloadTaskVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDownloadTask * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDownloadTask * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDownloadTask * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_nTaskID )( 
            IDownloadTask * This,
            /* [retval][out] */ LONG *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_nTaskID )( 
            IDownloadTask * This,
            /* [in] */ LONG newVal);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_nTotalLength )( 
            IDownloadTask * This,
            /* [retval][out] */ LONGLONG *pVal);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_nTotalLength )( 
            IDownloadTask * This,
            /* [in] */ LONGLONG newVal);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_nDownloadSize )( 
            IDownloadTask * This,
            /* [retval][out] */ LONGLONG *pVal);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_nDownloadSize )( 
            IDownloadTask * This,
            /* [in] */ LONGLONG newVal);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_fSpeed )( 
            IDownloadTask * This,
            /* [retval][out] */ FLOAT *pVal);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_fSpeed )( 
            IDownloadTask * This,
            /* [in] */ FLOAT pVal);
        
        END_INTERFACE
    } IDownloadTaskVtbl;

    interface IDownloadTask
    {
        CONST_VTBL struct IDownloadTaskVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDownloadTask_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDownloadTask_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDownloadTask_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDownloadTask_get_nTaskID(This,pVal)	\
    ( (This)->lpVtbl -> get_nTaskID(This,pVal) ) 

#define IDownloadTask_put_nTaskID(This,newVal)	\
    ( (This)->lpVtbl -> put_nTaskID(This,newVal) ) 

#define IDownloadTask_get_nTotalLength(This,pVal)	\
    ( (This)->lpVtbl -> get_nTotalLength(This,pVal) ) 

#define IDownloadTask_put_nTotalLength(This,newVal)	\
    ( (This)->lpVtbl -> put_nTotalLength(This,newVal) ) 

#define IDownloadTask_get_nDownloadSize(This,pVal)	\
    ( (This)->lpVtbl -> get_nDownloadSize(This,pVal) ) 

#define IDownloadTask_put_nDownloadSize(This,newVal)	\
    ( (This)->lpVtbl -> put_nDownloadSize(This,newVal) ) 

#define IDownloadTask_get_fSpeed(This,pVal)	\
    ( (This)->lpVtbl -> get_fSpeed(This,pVal) ) 

#define IDownloadTask_put_fSpeed(This,pVal)	\
    ( (This)->lpVtbl -> put_fSpeed(This,pVal) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDownloadTask_INTERFACE_DEFINED__ */


#ifndef __Idownloader2_INTERFACE_DEFINED__
#define __Idownloader2_INTERFACE_DEFINED__

/* interface Idownloader2 */
/* [unique][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_Idownloader2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4EED7C40-9520-4050-B7B4-C006EE6095D5")
    Idownloader2 : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddTask( 
            /* [in] */ BSTR szUrl,
            /* [in] */ BSTR pszSavePath,
            /* [in] */ BSTR pszSaveFileName,
            /* [in] */ LONG nThreadNum,
            /* [retval][out] */ LONG *nTaskID) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RemoveTask( 
            LONG nTaskID,
            /* [retval][out] */ VARIANT_BOOL *bRet) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE QueryTask( 
            /* [in] */ LONG nTaskID,
            /* [out][in] */ IDownloadTask **sDownloadTask,
            /* [retval][out] */ VARIANT_BOOL *bRet) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Pause( 
            /* [in] */ LONG nTaskID,
            /* [retval][out] */ VARIANT_BOOL *bRet) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Continue( 
            /* [in] */ LONG nTaskID,
            /* [retval][out] */ VARIANT_BOOL *bRet) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Cancel( 
            /* [in] */ LONG nTaskID,
            /* [retval][out] */ VARIANT_BOOL *bRet) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetCallBack( 
            /* [in] */ ICallBack *iCallBack,
            /* [retval][out] */ VARIANT_BOOL *bRet) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ReleaseCallBack( 
            /* [retval][out] */ VARIANT_BOOL *bRet) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct Idownloader2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            Idownloader2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            Idownloader2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            Idownloader2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            Idownloader2 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            Idownloader2 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            Idownloader2 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            Idownloader2 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AddTask )( 
            Idownloader2 * This,
            /* [in] */ BSTR szUrl,
            /* [in] */ BSTR pszSavePath,
            /* [in] */ BSTR pszSaveFileName,
            /* [in] */ LONG nThreadNum,
            /* [retval][out] */ LONG *nTaskID);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RemoveTask )( 
            Idownloader2 * This,
            LONG nTaskID,
            /* [retval][out] */ VARIANT_BOOL *bRet);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *QueryTask )( 
            Idownloader2 * This,
            /* [in] */ LONG nTaskID,
            /* [out][in] */ IDownloadTask **sDownloadTask,
            /* [retval][out] */ VARIANT_BOOL *bRet);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Pause )( 
            Idownloader2 * This,
            /* [in] */ LONG nTaskID,
            /* [retval][out] */ VARIANT_BOOL *bRet);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Continue )( 
            Idownloader2 * This,
            /* [in] */ LONG nTaskID,
            /* [retval][out] */ VARIANT_BOOL *bRet);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Cancel )( 
            Idownloader2 * This,
            /* [in] */ LONG nTaskID,
            /* [retval][out] */ VARIANT_BOOL *bRet);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetCallBack )( 
            Idownloader2 * This,
            /* [in] */ ICallBack *iCallBack,
            /* [retval][out] */ VARIANT_BOOL *bRet);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ReleaseCallBack )( 
            Idownloader2 * This,
            /* [retval][out] */ VARIANT_BOOL *bRet);
        
        END_INTERFACE
    } Idownloader2Vtbl;

    interface Idownloader2
    {
        CONST_VTBL struct Idownloader2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Idownloader2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define Idownloader2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define Idownloader2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define Idownloader2_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define Idownloader2_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define Idownloader2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define Idownloader2_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define Idownloader2_AddTask(This,szUrl,pszSavePath,pszSaveFileName,nThreadNum,nTaskID)	\
    ( (This)->lpVtbl -> AddTask(This,szUrl,pszSavePath,pszSaveFileName,nThreadNum,nTaskID) ) 

#define Idownloader2_RemoveTask(This,nTaskID,bRet)	\
    ( (This)->lpVtbl -> RemoveTask(This,nTaskID,bRet) ) 

#define Idownloader2_QueryTask(This,nTaskID,sDownloadTask,bRet)	\
    ( (This)->lpVtbl -> QueryTask(This,nTaskID,sDownloadTask,bRet) ) 

#define Idownloader2_Pause(This,nTaskID,bRet)	\
    ( (This)->lpVtbl -> Pause(This,nTaskID,bRet) ) 

#define Idownloader2_Continue(This,nTaskID,bRet)	\
    ( (This)->lpVtbl -> Continue(This,nTaskID,bRet) ) 

#define Idownloader2_Cancel(This,nTaskID,bRet)	\
    ( (This)->lpVtbl -> Cancel(This,nTaskID,bRet) ) 

#define Idownloader2_SetCallBack(This,iCallBack,bRet)	\
    ( (This)->lpVtbl -> SetCallBack(This,iCallBack,bRet) ) 

#define Idownloader2_ReleaseCallBack(This,bRet)	\
    ( (This)->lpVtbl -> ReleaseCallBack(This,bRet) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __Idownloader2_INTERFACE_DEFINED__ */



#ifndef __wac_downloaderLib_LIBRARY_DEFINED__
#define __wac_downloaderLib_LIBRARY_DEFINED__

/* library wac_downloaderLib */
/* [version][uuid] */ 


EXTERN_C const IID LIBID_wac_downloaderLib;

EXTERN_C const CLSID CLSID_downloader2;

#ifdef __cplusplus

class DECLSPEC_UUID("16ADDDC8-72EE-40E3-B366-523DC6756905")
downloader2;
#endif

EXTERN_C const CLSID CLSID_CallBack;

#ifdef __cplusplus

class DECLSPEC_UUID("CFA4BB43-198F-491F-941B-39C6A33E5C00")
CallBack;
#endif

EXTERN_C const CLSID CLSID_DownloadTask;

#ifdef __cplusplus

class DECLSPEC_UUID("E422383B-EB02-4580-B168-588611CD75F2")
DownloadTask;
#endif
#endif /* __wac_downloaderLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


